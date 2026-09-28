#!/usr/bin/env python3
"""0.1a: rename one audio/MIDI event in a separate .song copy, never the source."""

import argparse
from dataclasses import dataclass
from hashlib import sha256
import os
from pathlib import Path
import shutil
import tempfile
from xml.parsers import expat
from xml.sax.saxutils import escape
from zipfile import BadZipFile, ZipFile
import copy


SONG_XML = "Song/song.xml"
MAX_XML = 64 * 1024 * 1024
EVENT_TYPES = ("AudioEvent", "MusicPart")


@dataclass(frozen=True)
class EventSelector:
    track_id: str
    event_type: str
    old_name: str
    start: str
    length: str
    time_format: str = ""
    clip_id: str | None = None


def _attribute_span(tag: bytes, key: bytes) -> tuple[int, int, int]:
    """Return the value's byte offsets and quote, without changing other XML."""
    i = 1
    while i < len(tag) and tag[i] not in b" \t\r\n/>":
        i += 1
    found = []
    while i < len(tag):
        while i < len(tag) and tag[i] in b" \t\r\n":
            i += 1
        if i >= len(tag) or tag[i] in b"/>":
            break
        start = i
        while i < len(tag) and tag[i] not in b"= \t\r\n/>":
            i += 1
        name = tag[start:i]
        while i < len(tag) and tag[i] in b" \t\r\n":
            i += 1
        if i >= len(tag) or tag[i] != ord("="):
            raise ValueError("Invalid XML attribute syntax")
        i += 1
        while i < len(tag) and tag[i] in b" \t\r\n":
            i += 1
        if i >= len(tag) or tag[i] not in (ord('"'), ord("'")):
            raise ValueError("Invalid XML attribute quotation")
        quote = tag[i]
        i += 1
        value_start = i
        while i < len(tag) and tag[i] != quote:
            i += 1
        if i == len(tag):
            raise ValueError("Unterminated XML attribute")
        if name == key:
            found.append((value_start, i, quote))
        i += 1
    if len(found) != 1:
        raise ValueError("Selected event must have exactly one name attribute")
    return found[0]


def _opening_tag(data: bytes, offset: int) -> bytes:
    if data[offset:offset + 1] != b"<":
        raise ValueError("Cannot locate selected XML element")
    quote = None
    for i in range(offset + 1, len(data)):
        char = data[i]
        if quote is not None:
            if char == quote:
                quote = None
        elif char in (ord('"'), ord("'")):
            quote = char
        elif char == ord(">"):
            return data[offset:i + 1]
    raise ValueError("Unterminated XML element")


def _replace_event_name(data: bytes, selector: EventSelector, new_name: str) -> bytes:
    if selector.event_type not in EVENT_TYPES:
        raise ValueError("Only AudioEvent and MusicPart can be renamed")
    if not new_name or len(new_name) > 1024 or any(ord(c) < 32 for c in new_name):
        raise ValueError("Invalid new clip name")
    if b"<!DOCTYPE" in data.upper() or b"<!ENTITY" in data.upper():
        raise ValueError("External entities and DTDs are not supported")

    parser = expat.ParserCreate()
    stack: list[tuple[str, dict[str, str]]] = []
    matches: list[tuple[int, int, int]] = []
    root = None

    def begin(tag: str, attrs: dict[str, str]) -> None:
        nonlocal root
        if root is None:
            root = tag
        if (tag == selector.event_type and len(stack) >= 3
                and stack[-1][0] == "List" and stack[-1][1].get("x:id") == "Events"
                and stack[-2][0] == "MediaTrack"
                and stack[-2][1].get("trackID", "") == selector.track_id
                and stack[-3][0] == "List" and stack[-3][1].get("x:id") == "Tracks"
                and attrs.get("name", "") == selector.old_name
                and attrs.get("start", "0") == selector.start
                and attrs.get("length", "0") == selector.length
                and attrs.get("timeFormat", "") == selector.time_format
                and (selector.clip_id is None or attrs.get("clipID") == selector.clip_id)):
            offset = parser.CurrentByteIndex
            tag_bytes = _opening_tag(data, offset)
            first, last, quote = _attribute_span(tag_bytes, b"name")
            matches.append((offset + first, offset + last, quote))
        stack.append((tag, attrs))

    def end(_tag: str) -> None:
        stack.pop()

    parser.StartElementHandler = begin
    parser.EndElementHandler = end
    parser.Parse(data, True)
    if root != "Song":
        raise ValueError("Unsupported Song/song.xml root")
    if len(matches) != 1:
        raise ValueError(f"Expected one matching event, found {len(matches)}; no copy written")
    first, last, quote = matches[0]
    escaped = escape(new_name, {'"': "&quot;", "'": "&apos;"} if quote == ord('"')
                     else {"'": "&apos;"}).encode("utf-8")
    changed = data[:first] + escaped + data[last:]
    expat.ParserCreate().Parse(changed, True)
    return changed


def _file_digest(path: Path) -> bytes:
    digest = sha256()
    with path.open("rb") as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.digest()


def _entry_digest(archive: ZipFile, info) -> bytes:
    digest = sha256()
    with archive.open(info) as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.digest()


def _verify(original: Path, output: Path, expected_xml: bytes) -> None:
    with ZipFile(original) as before, ZipFile(output) as after:
        a, b = before.infolist(), after.infolist()
        if len(a) != len(b) or before.comment != after.comment:
            raise ValueError("Archive entries or comment changed")
        for old, new in zip(a, b):
            fields = ("filename", "date_time", "compress_type", "comment", "extra",
                      "external_attr", "internal_attr", "create_system")
            if any(getattr(old, key) != getattr(new, key) for key in fields):
                raise ValueError(f"ZIP metadata changed: {old.filename}")
            if old.filename == SONG_XML:
                with after.open(new) as stream:
                    if stream.read() != expected_xml:
                        raise ValueError("Edited XML failed verification")
            elif old.file_size != new.file_size or _entry_digest(before, old) != _entry_digest(after, new):
                raise ValueError(f"Unrelated archive entry changed: {old.filename}")


def write_song_copy(source: Path, destination: Path, selector: EventSelector, new_name: str) -> None:
    source, destination = Path(source), Path(destination)
    if source.suffix.lower() != ".song" or destination.suffix.lower() != ".song":
        raise ValueError("Both paths must end in .song")
    if source.resolve() == destination.resolve() or destination.exists():
        raise ValueError("Output must be a new path, separate from the source")
    if not source.is_file() or not destination.parent.is_dir():
        raise ValueError("Source file or destination directory is missing")
    initial_stat, initial_digest = source.stat(), _file_digest(source)
    temporary = None
    try:
        with ZipFile(source) as original:
            entries = original.infolist()
            if len({item.filename for item in entries}) != len(entries):
                raise ValueError("Duplicate ZIP paths are not supported")
            if sum(item.filename == SONG_XML for item in entries) != 1:
                raise ValueError("Missing Song/song.xml")
            if not any(item.filename == "metainfo.xml" for item in entries):
                raise ValueError("Missing metainfo.xml")
            song_info = original.getinfo(SONG_XML)
            if song_info.file_size > MAX_XML:
                raise ValueError("Song/song.xml is too large")
            changed_xml = _replace_event_name(original.read(song_info), selector, new_name)
            with tempfile.NamedTemporaryFile(prefix=".arranger-copy-", suffix=".song",
                                             dir=destination.parent, delete=False) as tmp:
                temporary = Path(tmp.name)
            with ZipFile(temporary, "w", allowZip64=True) as output:
                output.comment = original.comment
                for info in entries:
                    with output.open(copy.copy(info), "w") as writer:
                        if info.filename == SONG_XML:
                            writer.write(changed_xml)
                        else:
                            with original.open(info) as reader:
                                shutil.copyfileobj(reader, writer, 1024 * 1024)
        _verify(source, temporary, changed_xml)
        current_stat = source.stat()
        if (current_stat.st_size != initial_stat.st_size
                or current_stat.st_mtime_ns != initial_stat.st_mtime_ns
                or _file_digest(source) != initial_digest):
            raise ValueError("Source changed during copy; no output written")
        os.link(temporary, destination)  # Exclusive: never overwrite a concurrent file.
    finally:
        if temporary is not None:
            temporary.unlink(missing_ok=True)


def main() -> None:
    cli = argparse.ArgumentParser(description=__doc__)
    cli.add_argument("source", type=Path)
    cli.add_argument("destination", type=Path)
    cli.add_argument("--track-id", required=True)
    cli.add_argument("--type", choices=EVENT_TYPES, required=True)
    cli.add_argument("--old-name", required=True)
    cli.add_argument("--start", required=True)
    cli.add_argument("--length", required=True)
    cli.add_argument("--time-format", default="")
    cli.add_argument("--clip-id")
    cli.add_argument("--new-name", required=True)
    args = cli.parse_args()
    selector = EventSelector(args.track_id, args.type, args.old_name, args.start,
                             args.length, args.time_format, args.clip_id)
    try:
        write_song_copy(args.source, args.destination, selector, args.new_name)
    except (OSError, BadZipFile, expat.ExpatError, ValueError, NotImplementedError) as error:
        cli.exit(1, f"No copy created: {error}\n")
    print(f"Verified copy created: {args.destination}")


if __name__ == "__main__":
    main()

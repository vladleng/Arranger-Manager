#!/usr/bin/env python3
"""Read Info and Notes from a saved Studio Pro .song archive without editing it."""

import argparse
import json
from pathlib import Path
from xml.etree import ElementTree
from zipfile import BadZipFile, ZipFile


MAX_MEMBER_BYTES = 2 * 1024 * 1024


def read_member(archive, name, limit=MAX_MEMBER_BYTES):
    members = [item for item in archive.infolist() if item.filename == name]
    if not members:
        return None
    if len(members) != 1 or members[0].file_size > limit:
        raise ValueError(f"Invalid or oversized archive entry: {name}")
    with archive.open(members[0]) as stream:
        content = stream.read(limit + 1)
    if len(content) > limit:
        raise ValueError(f"Archive entry is too large: {name}")
    return content


def parse_xml(content, name):
    if content is None:
        return None
    if b"<!DOCTYPE" in content.upper() or b"<!ENTITY" in content.upper():
        raise ValueError(f"XML declarations are not supported: {name}")
    return ElementTree.fromstring(content)


def read_metadata(path):
    with ZipFile(path) as archive:
        meta = parse_xml(read_member(archive, "metainfo.xml"), "metainfo.xml")
        if meta is None or meta.tag != "MetaInformation":
            raise ValueError("Missing Studio Pro metainfo.xml")
        attributes = {}
        for item in meta.findall("Attribute"):
            key = item.get("id")
            if key:
                attributes[key] = item.get("value", "")

        notes_name = attributes.get("Document:Notes")
        # The manifest points to the notebook text. Do not follow arbitrary
        # paths or read plug-in presets from a malformed project archive.
        if notes_name and (notes_name != "notes.txt"):
            raise ValueError("Unexpected Document:Notes location")
        notes = read_member(archive, notes_name) if notes_name else None
        notepad = parse_xml(read_member(archive, "notepad.xml"), "notepad.xml")
        track_notes = []
        if notepad is not None:
            for item in notepad.findall("NotepadItem"):
                track_notes.append({
                    "id": item.get("id", ""),
                    "title": item.get("title", ""),
                    "text": item.get("text", ""),
                })
        return {
            "schema": "arranger-manager-saved-song-metadata-v1",
            "source": "saved .song archive",
            "document": {k[9:]: v for k, v in attributes.items() if k.startswith("Document:")},
            "media": {k[6:]: v for k, v in attributes.items() if k.startswith("Media:")},
            "other": {k: v for k, v in attributes.items()
                      if not k.startswith(("Document:", "Media:"))},
            "notes": notes.decode("utf-8-sig") if notes is not None else None,
            "trackNotes": track_notes,
        }


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("song", type=Path, help="saved .song file")
    parser.add_argument("-o", "--output", type=Path, help="JSON output (default: stdout)")
    args = parser.parse_args()
    try:
        payload = read_metadata(args.song)
    except (OSError, BadZipFile, ValueError, ElementTree.ParseError) as error:
        parser.exit(1, f"Cannot read song metadata: {error}\n")
    result = json.dumps(payload, ensure_ascii=False, indent=2) + "\n"
    if args.output:
        args.output.write_text(result, encoding="utf-8")
    else:
        print(result, end="")


if __name__ == "__main__":
    main()

#!/usr/bin/env python3
"""Inventory tracks and event blocks in a saved Studio Pro .song archive."""

import argparse
import json
from pathlib import Path
import re
from xml.etree import ElementTree
from zipfile import BadZipFile, ZipFile

from read_song_metadata import read_member


MAX_SONG_XML_BYTES = 64 * 1024 * 1024


def read_inventory(path):
    with ZipFile(path) as archive:
        data = read_member(archive, "Song/song.xml", MAX_SONG_XML_BYTES)
        if data is None:
            raise ValueError("Missing Song/song.xml")
        if b"<!DOCTYPE" in data.upper() or b"<!ENTITY" in data.upper():
            raise ValueError("XML declarations are not supported")
        # Studio Pro 8.1.2 uses unbound x:id attributes. Normalize them only
        # in memory so a standard XML parser can inspect the saved snapshot.
        data = re.sub(rb"(?<=\s)x:id=", b"x_id=", data)
        root = ElementTree.fromstring(data)
        if root.tag != "Song":
            raise ValueError("Unexpected Song/song.xml root")

        tracks = []
        for track in root.iter("MediaTrack"):
            events = []
            for event_list in track.findall("List"):
                if event_list.get("x_id") != "Events":
                    continue
                for event in event_list:
                    events.append({"type": event.tag, "attributes": dict(event.attrib)})
            tracks.append({
                "id": track.get("trackID"),
                "name": track.get("name"),
                "mediaType": track.get("mediaType"),
                "events": events,
            })
        return {
            "schema": "arranger-manager-saved-song-inventory-v1",
            "source": "saved .song archive; raw event time units",
            "tracks": tracks,
        }


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("song", type=Path)
    parser.add_argument("-o", "--output", type=Path)
    args = parser.parse_args()
    try:
        payload = read_inventory(args.song)
    except (OSError, BadZipFile, ValueError, ElementTree.ParseError) as error:
        parser.exit(1, f"Cannot read song inventory: {error}\n")
    result = json.dumps(payload, ensure_ascii=False, indent=2) + "\n"
    if args.output:
        args.output.write_text(result, encoding="utf-8")
    else:
        print(result, end="")


if __name__ == "__main__":
    main()

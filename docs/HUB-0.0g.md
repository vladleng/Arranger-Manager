# Hub 0.0g — project tree preview

This read-only Windows package contains only **Arranger Manager Hub.vst3**. Replace the earlier Hub VST3, restart Studio Pro and open the same saved `.song` in Hub. ARA Inspector is not needed.

## Inspect the hierarchy

- Project row expands into **Tracks**, **Arrangement** and **Markers**.
- Each track expands into its ordinary audio or MIDI clips, sorted by saved start position. Tracks with no clips remain visible.
- Arrangement sections expand into references to clips whose saved numeric intervals overlap the section. These rows refer to the same clips shown under Tracks; they do not create or change DAW events. Marker rows show saved point positions.
- Parent rows show **DONE / tagged** counts, not an overall completion percentage. Untagged clips are not silently treated as TODO. Positions and lengths are raw saved units.
- Clicking a disclosure row collapses/expands it. The choice persists while the plug-in editor stays open and the same project reloads after Save.

In the supplied test project, expect **5 tracks, 8 audio events, 2 MIDI parts, 4 arrangement sections and 2 markers** (Start/End). Change an event name in Studio Pro and save: Hub should update the tree within about one second. **Refresh** forces a read.

## Status and priority

| Status | Colour | Priority | Colour |
| --- | --- | --- | --- |
| POOL | Violet | P1 (highest) | Red |
| TODO | Gray | P2 | Orange |
| WIP | Blue | P3 | Yellow |
| DRAFT | Cyan | P4 (lowest) | Gray |
| WAIT | Amber | | |
| DONE | Green | | |
| BLOCKED | Red | | |

Use `WAIT | P3 | optional note` for new clip names. Older `REVIEW | P3` names are still read as WAIT without changing the native clip name. P0 is no longer a supported priority; rename old P0 tags in Studio Pro. The sample's old REVIEW clip will therefore display a WAIT badge while its unchanged raw name still reads REVIEW.

This preview reads `metainfo.xml`, `notes.txt` and `Song/song.xml` from the saved project. The section-to-clip links compare raw numeric saved positions; projects with different time formats or nonstandard structures require further verification. Hub never modifies `.song` and does not display unsaved edits.

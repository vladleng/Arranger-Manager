# Hub 0.0h — arrangement labels and track notes

This read-only Windows package contains **Arranger Manager Hub.vst3**. Replace the previous Hub VST3, restart Studio Pro and open the same saved `.song` in Hub. ARA Inspector is not needed.

## Tree and columns

- Project → Tracks → audio/MIDI clips; Markers remain a project-level group. The separate Arrangement group from 0.0g is removed.
- Each clip displays `[Section]` as a colored label before its name when its saved interval overlaps an arrangement event. The label uses the event's color from `Song/song.xml`. A clip spanning two sections receives two labels. When the native clip name already begins with that same `[Section]`, the repeated plain-text prefix is omitted in the display.
- **Notes** appears after **Priority**. A track's row displays the matching `NotepadItem` text from `notepad.xml`, joined by its stable `trackID`, including empty tracks. Long notes fit the available column width. **Position** remains to the right on wide windows.
- Status and priority colors are unchanged from 0.0g: `WAIT` replaces `REVIEW` in the display; P1 red, P2 orange, P3 yellow, P4 gray. Existing `REVIEW` clip names are read as WAIT without rewriting them.
- Clicking project, Tracks, individual tracks, or Markers collapses or expands them. The current choice persists across refreshes of the same project while the editor is open.

## Check in Studio Pro

1. Open the supplied test project and expand Drums, Bass and Mai Tai. Expect 5 tracks, 10 clips (8 audio, 2 MIDI), 4 arrangement sections represented by colored clip labels, and 2 markers.
2. The Drums track has a saved track note; it should appear in **Notes** on the Drums row, not on its clips. Other tracks with empty notes show an empty field.
3. Rename an arrangement event, recolor it, or edit a track note. Save the project. Hub should update from the saved `.song` within about a second. **Refresh** forces a read.
4. Resize the attached window. At narrow widths the Position column hides to leave room for Notes; the font stays the same size.

Matching compares raw numeric intervals from the saved project. Projects with different time formats or nonstandard structures may need further verification. Hub does not edit `.song` and does not display unsaved changes.

# Hub 0.0i — cleaner clip names and track colors

This read-only Windows package contains **Arranger Manager Hub.vst3**. Replace the previous Hub VST3, restart Studio Pro, and open the saved `.song` in Hub.

- A track row now has a colored circle immediately before its name. The color comes from the track's `MediaTrack color` in the saved project. If a track has no saved color, the circle uses a neutral fallback.
- A clip whose name is a valid arrangement tag no longer repeats `STATUS | PRIORITY` in the Name column. For `WIP | P2 | record fills`, the Name cell reads `record fills`, while WIP and P2 remain in their own columns. For `WIP | P2` with no extra title, the Name cell reads `Audio` or `MIDI`. Untagged names are preserved, as are the colored `[Section]` labels from arrangement events.
- Colored section labels use the saved `ArrangerEvent color` (or inherited `ArrangerTrack color` where no individual color is stored). Hub reads a **saved** `.song`; changes visible in Studio Pro while the project title shows `*` are not available until Save.
- The Notes column still displays track notes from `notepad.xml`; no project content is written by Hub.

## Check in Studio Pro

1. Save the project after recoloring Intro, Verse, Chorus or Ending, then confirm the colored labels match those regions. Hub reloads the saved file within about one second; **Refresh** forces a read.
2. Verify colored track circles beside Drums, Bass and Mai Tai and compare with their saved track colors.
3. Verify `TODO | P2` appears as `[Intro] Audio` with TODO and P2 in their columns; `REVIEW | P3` remains a WAIT badge and appears as `[Chorus] Audio`. A native `[Verse 2]` name remains visible alongside the current section label.

This preview still compares raw numeric saved positions to associate clips with sections. Host-only visual changes and unsaved edits remain outside its snapshot.

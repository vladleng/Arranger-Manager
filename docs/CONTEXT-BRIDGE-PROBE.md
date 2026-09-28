# Studio Pro context bridge: read-only probe

The public VST3 host extensions expose channel and current event context but no full marker or Arranger-section collection. This diagnostic uses Studio Pro's undocumented scripting surface. Its behavior must be validated in the target Studio Pro 8 build before incorporating it into the plug-in.

## Installation

Copy `ArrangerManagerContextProbe.package` to `C:\Program Files\Fender\Studio Pro 8\Scripts\` (administrator rights may be needed) and restart Studio Pro. The `.package` is a ZIP archive with `metainfo.xml`, `classfactory.xml`, and `main.js` at its root. The script adds three actions under **Arranger Manager**. Installation and the scripting API are based on a community-derived reference and may differ in your build.

## Test

1. In the test song, select the Arranger sections on the Arranger track. Run **Export Arranger Sections** from the Actions menu. The script writes `Arranger_Manager_Arranger.json` into the Studio Pro **User Data Location** (Preferences → Locations → User Data).
2. Select the markers on the Marker track. Run **Export Markers**. It writes `Arranger_Manager_Markers.json` in the same location.
3. Check `count`, `events`, and `error` in each file. If the start/end markers cannot be selected or are absent, record that fact; the loop boundaries are exported separately under `cursor` and are not assumed to be song start/end.
4. To inspect project-level access, put distinctive text in **Session Information → Info** and **Notes**, select any event and run **Export Project Metadata**. It writes `Arranger_Manager_Metadata.json` to the same folder. Compare the values in `document` and `objects` with the text you entered. `available: true` only means that a host object exists; it does not mean it contains the Notes text. The `Inspector.Notes` and `Toolbar.InfoView` objects are UI controls and must not be confused with the session notebook.

All actions only inspect the current selection or active document and write diagnostic files. They do not rename, recolor, create, or move any DAW objects. The JSON is not yet consumed by the ARA plug-in. It is a capability check before adding a live transport and synchronization rules. Files may reveal song structure, notes, and folder paths; review their contents before sharing.

The public PreSonus/Fender VST3 context extension lists document identity, name, document folder, and audio folder, but no Info or Notes fields. The scripting lookup above did not expose those fields in the first Studio Pro 8 test. A read-only parser of the saved `.song` archive provides the last saved state, not unsaved edits.

## Saved project metadata (Studio Pro 8.1.2 sample)

The supplied `Arranger Manager.song` is a ZIP archive. Its `metainfo.xml` has `Document:*` and `Media:*` attributes, including separate `Document:Title` (file/session title) and `Media:Title` (Info title). `Document:Notes` points to `notes.txt`, which contains the session notebook. `notepad.xml` contains channel/track notes as `NotepadItem` elements with an ID, title, and text; a track ID in `Song/song.xml` matches the corresponding notepad ID in this sample. These are distinct from event-level ARA state and should not be conflated with `Inspector.Notes` UI visibility.

To extract the saved snapshot without opening or changing the song:

```sh
python tools/read_song_metadata.py "Arranger Manager.song" -o metadata.json
```

The script reads only three small archive members (`metainfo.xml`, referenced `notes.txt`, `notepad.xml`) and emits `document`, `media`, `notes`, and `trackNotes`. It does not extract presets, media, or artwork. This verifies saved-file access on the supplied 8.1.2 sample; for a live bridge, use the host document identity/folder to locate the current `.song` and refresh only after a save. Unsaved edits to Info/Notes cannot be inferred from the saved file. The file format is not documented as a stable public API, so handle absent/changed entries and test future Studio Pro releases.

## Reverse synchronization feasibility

On an isolated copy of the supplied `.song`, changing `Media:Title` in `metainfo.xml`, the text of `notes.txt`, and the Drums `NotepadItem` text in `notepad.xml` produced a valid ZIP with parseable XML. The other 45 archive members had identical uncompressed contents. **Studio Pro has not yet opened this edited copy**, so compatibility and behavior after another DAW save remain unverified.

For notes owned by Arranger Manager, the preferred round trip is: edit them in an external Hub data store, transfer them to the running Hub plug-in, mark its state dirty, and let Studio Pro serialize that state on Save. This does not automatically populate Studio Pro's own Info/Notes fields. To write those native fields offline, close the song first, match track notes by persistent `trackID`/`NotepadItem.id` (not by potentially duplicated or renamed track names), compare the source file revision before writing, keep a backup, modify only the three relevant archive members in a temporary file, verify the result, then atomically replace or offer a separate edited copy. If Studio Pro has the original song open, its next Save can overwrite external changes; direct modification of an open `.song` must not be used as a live synchronization mechanism.

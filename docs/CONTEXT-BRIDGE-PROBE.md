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

The public PreSonus/Fender VST3 context extension lists document identity, name, document folder, and audio folder, but no Info or Notes fields. The scripting lookup above is exploratory and does not prove those fields are exposed. If they are absent, a read-only parser of the saved `.song` archive is the next route; it reflects the last saved state, not unsaved edits. That route needs a representative Studio Pro 8 `.song` file to identify the exact XML fields.

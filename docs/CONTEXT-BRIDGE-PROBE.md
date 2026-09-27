# Studio Pro context bridge: read-only probe

The public VST3 host extensions expose channel and current event context but no full marker or Arranger-section collection. This diagnostic uses Studio Pro's undocumented scripting surface. Its behavior must be validated in the target Studio Pro 8 build before incorporating it into the plug-in.

## Installation

Copy `ArrangerManagerContextProbe.package` to `C:\Program Files\Fender\Studio Pro 8\Scripts\` (administrator rights may be needed) and restart Studio Pro. The `.package` is a ZIP archive with `metainfo.xml`, `classfactory.xml`, and `main.js` at its root. The script adds two actions under **Arranger Manager**. Installation and the scripting API are based on a community-derived reference and may differ in your build.

## Test

1. In the test song, select the Arranger sections on the Arranger track. Run **Export Arranger Sections** from the Actions menu. The script writes `Arranger_Manager_Arranger.json` into the Studio Pro **User Data Location** (Preferences → Locations → User Data).
2. Select the markers on the Marker track. Run **Export Markers**. It writes `Arranger_Manager_Markers.json` in the same location.
3. Check `count`, `events`, and `error` in each file. If the start/end markers cannot be selected or are absent, record that fact; the loop boundaries are exported separately under `cursor` and are not assumed to be song start/end.

Both actions only inspect the current selection and write diagnostic files. They do not rename, recolor, create, or move any DAW objects. The JSON is not yet consumed by the ARA plug-in. It is a capability check before adding a live transport and synchronization rules. Files may reveal song structure; review their contents before sharing.

# 0.0e — Arranger and marker context import

This test build adds a manual read-only bridge from Studio Pro's scripting actions to the ARA Inspector. The two script exports were validated in Studio Pro 8: four Arranger sections and special Start/End markers with timing. The script returns selected objects, so select all relevant sections or markers before each export.

1. Install both VST3 bundles. Install `ArrangerManagerContextProbe.package` in `C:\Program Files\Fender\Studio Pro 8\Scripts\` and restart Studio Pro.
2. Select all Arranger sections and run **Export Arranger Sections**; select Start, End and any other markers and run **Export Markers**. They write `Arranger_Manager_Arranger.json` and `Arranger_Manager_Markers.json` in Studio Pro's User Data Location (Preferences → Locations → User Data).
3. Open the **Arranger Manager Inspector** ARA Event FX window. Click **Import JSON** and select the Arranger JSON. Click it again and select the Markers JSON.
4. The event list now prefixes each event note with all overlapping Arranger sections. The diagnostic report includes imported sections and marker positions. When both Start and End were imported, the summary shows the song range.

The importer checks the schema and times and sorts sections by their start, since the script may return them in reverse timeline order. It keeps the snapshots only in the currently open editor. After changes in the DAW, re-export and re-import; closing this editor loses the imported context. Exporting a subset replaces the JSON file with that subset. There is no continuous synchronization, host editing, or persistence in the project yet.

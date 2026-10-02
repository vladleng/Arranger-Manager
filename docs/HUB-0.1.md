# Arranger Manager 0.1 — Windows app

Extract the ZIP and run `Arranger Manager.exe`. No VST3 installation is required.

- Click **Add .song** to import saved Studio Pro projects. Use **New folder** to group songs. Right-click a song to move or remove it; removing a catalog entry does not delete the `.song` file.
- Click a song to expand it. Tracks and Markers stay collapsed until you open them; individual tracks stay collapsed until you open them, including after restarting the app.
- Click the **Status** cell on a song or track to choose POOL, TODO, WIP, DRAFT, WAIT, DONE or BLOCKED. Use **Clear status** to remove it. Song and track statuses are independent; a song can be WAIT while a track is WIP. These manually chosen statuses persist in the app catalog on this computer, not in the `.song` file.
- **PROG** shows `DONE / all clips` for each track and song, and the aggregate for folders. Untagged clips and clips with any other status count as unfinished. Changing a song or track status does not change its progress.
- Clip statuses come from their names in the saved `.song`. Save edits in Studio Pro to refresh the app's snapshot. This version reads project files but does not edit closed projects.

Existing 0.0m catalogs load with no song or track statuses selected. Missing song files remain visible with an error so the entry can be removed or found again.

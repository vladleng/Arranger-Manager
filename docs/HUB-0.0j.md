# Hub 0.0j — status-only clips and arrangement dots

This read-only Windows package contains **Arranger Manager Hub.vst3**. Replace the previous Hub VST3, restart Studio Pro, and open a saved `.song` in Hub.

- The project view has **Name, Status, Notes** and, on wider windows, **Position**. Priority is reserved for songs in the future cross-project Hub; it no longer occupies a column or badge on tracks or clips.
- Use `WIP` or `WIP | task text` for new clip names. Existing `WIP | P2` and `WIP | P2 | task text` names remain readable without changing the Studio Pro project: the status is shown, the embedded priority is ignored in this view, and the optional text becomes the clip's display name. `REVIEW` still displays as WAIT.
- Arrangement section colors are small circles before plain `[Section]` names, rather than colored blocks. A clip overlapping two sections shows two circle/name pairs. Track colors remain circles on track rows.
- Notes come from the track notepad. Hub reads the latest saved `.song`; changes pending in Studio Pro do not appear until Save.

## Check in Studio Pro

1. Verify the Priority column and P badges are gone, while TODO, WIP, WAIT and other status badges remain.
2. Rename a clip to `WIP | сделать партии`, save the project, and verify its displayed title is `сделать партии` and its status is WIP. Older `WIP | P2 | сделать партии` should display the same title and status.
3. Confirm `[Intro]`, `[Verse]`, `[Chorus]` and `[Ending]` are plain text with small saved-color circles directly to their left. Save the project after recoloring an arrangement event to update Hub.

Song-level priorities belong to the planned catalog of projects and are not inferred from clip names.

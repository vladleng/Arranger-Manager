# Hub 0.0k fix1 — clip notes, media type, and saved colors

This read-only Windows package contains **Arranger Manager Hub.vst3**. Replace the previous Hub VST3, restart Studio Pro, and open a saved `.song` in Hub.

- The project view has **Name, Type, Status, Notes** and, on wider windows, **Position**.
- Name shows arrangement sections as colored circles followed by `[Section]`. A clip without a matching section shows `Clip`.
- The clip name or optional text after `STATUS |` appears in Notes. Status and legacy priority tokens are removed. If the native clip name already starts with the matching `[Section]`, that prefix is not repeated in Notes. Track Notes still come from the track notepad.
- Type shows a blue waveform for audio and an orange keyboard for MIDI. Track color circles remain beside track names; track icon extraction is deferred.
- Section and track colors use Studio Pro's saved ABGR channel order. Arrangement events without their own color inherit the arrangement track color. Studio Pro's unsaved edits do not appear until Save.

## Check in Studio Pro

1. Rename one audio clip to `WIP | сделать партии` and one MIDI part to `TODO | проверить ноты`, then save the project. Verify their text in Notes, corresponding statuses, and blue/orange Type icons.
2. Check a clip named `[Verse 1]` against the matching arrangement section. `[Verse 1]` should appear once under Name, with an empty Notes field.
3. Compare Intro, Verse, Chorus, Ending, Bass and the guitar track circles with Studio Pro. The existing saved values should match the palette without a new save. New color edits still require Save before Hub updates.

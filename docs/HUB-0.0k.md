# Hub 0.0k — clip notes and media type

This read-only Windows package contains **Arranger Manager Hub.vst3**. Replace the previous Hub VST3, restart Studio Pro, and open a saved `.song` in Hub.

- The project view has **Name, Type, Status, Notes** and, on wider windows, **Position**.
- Name shows arrangement sections as colored circles followed by `[Section]`. A clip without a matching section shows `Clip`.
- The clip name or optional text after `STATUS |` appears in Notes. Status and legacy priority tokens are removed. If the native clip name already starts with the matching `[Section]`, that prefix is not repeated in Notes. Track Notes still come from the track notepad.
- Type shows a blue waveform for audio and an orange keyboard for MIDI. Track color circles remain beside track names; track icon extraction is deferred.
- Section colors come from the last saved `.song`. Studio Pro's unsaved edits do not appear until Save. Arrangement events without their own saved color inherit the arrangement track color.

## Check in Studio Pro

1. Rename one audio clip to `WIP | сделать партии` and one MIDI part to `TODO | проверить ноты`, then save the project. Verify their text in Notes, corresponding statuses, and blue/orange Type icons.
2. Check a clip named `[Verse 1]` against the matching arrangement section. `[Verse 1]` should appear once under Name, with an empty Notes field.
3. Recolor Intro, Verse and Ending in Studio Pro, save the project (the `*` should disappear from its title), then check their circles in Hub. If they still differ, send the newly saved `.song`.

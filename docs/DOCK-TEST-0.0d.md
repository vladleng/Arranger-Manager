# 0.0d — ARA window resize and dock test

Install both VST3 plug-ins. **Arranger Manager Inspector** is the ARA Event FX that can be attached to Studio Pro's ARA editor panel. **Arranger Manager Hub** is an optional ordinary insert and has only a floating plug-in window in this workflow; it is not a substitute for the attached ARA editor.

Put the Inspector Event FX on each marked audio event with a real audio source. Open one Inspector window, resize it while floating, then attach it to Studio Pro's ARA panel. Change the dock panel width and height. The fonts, badges, and row height remain constant. On narrow panels the status badges wrap; on short panels the raw diagnostic report is hidden to give the event list room. The report reappears when the panel is tall enough and can still be copied with **Copy report**.

The previous version crashed Studio Pro while attaching the ARA editor in the reported setup. This build addresses layout handling, but Studio Pro testing is required to establish whether the crash is fixed. If it still crashes, save the new minidump and describe whether the floating resize worked immediately before attachment.

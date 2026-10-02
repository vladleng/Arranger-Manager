# Arranger Manager Hub 0.0c — Studio Pro test

This package contains two VST3 plug-ins. Install both `.vst3` folders in the usual VST3 location and rescan plug-ins in Studio Pro.

1. Put **Arranger Manager Inspector** as an ARA Event FX on each marked audio event. Use a real silent WAV source for service events. Copying or splitting the events should copy the Event FX too. The Event FX may be disabled if Studio Pro keeps its ARA model active in your project.
2. Put **Arranger Manager Hub** as an ordinary insert on an audio track and open its floating window. Studio Pro attaches the ARA Event FX editor to its ARA panel, not this ordinary insert. The Hub only passes audio through; it reads the ARA region snapshot from the Inspector in the same Studio Pro process.
3. Name the marked events `STATUS | P1 | optional note`, for example `WIP | P1 | chorus brass`. The track name supplies the instrument group. Use the Studio Pro event color for status; priority has its own badge in the Hub. See [the color contract](MAP-PREVIEW-0.0b.md).
4. Try resizing the floating Hub window. Its fonts and badges stay at the same size; the available list area changes, and status badges wrap in a narrow window.

The prior ARA Event FX window caused a Studio Pro access violation during one docking attempt, but a subsequent attempt successfully attached the 0.0b Inspector. The Hub provides an optional floating view; the ARA Inspector's own resizing is addressed in 0.0d.

This preview displays up to 256 ARA regions from the current process. It does not change host events, persist independent task state, calculate coverage, or isolate multiple open songs. Close and reopen the Hub after replacing the plug-ins if Studio Pro has cached the old binaries.

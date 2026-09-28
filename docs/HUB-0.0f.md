# Hub 0.0f — saved Studio Pro project reader

This test build contains only **Arranger Manager Hub.vst3**. Add it as an ordinary insert on any track; it passes audio through and needs no ARA Event FX.

1. Open Hub, click **Open .song**, and choose the `.song` file of the currently open Studio Pro project. The plug-in stores only this path in its own state; the project content remains in the native Studio Pro file.
2. Hub should show the project's title, Info title/artist, session Notes, track count and all main-timeline audio/MIDI events in the supplied test song: **5 tracks, 8 audio events, 2 MIDI parts**. `POOL | P2` should have a violet POOL badge and a blue P2 badge; `DONE | P1` should have a green DONE badge and an orange P1 badge. Ordinary clip names remain UNMARKED.
3. Rename one event in Studio Pro and **save the song**. Hub checks the file timestamp/size once per second and reloads the saved snapshot. **Refresh** also forces a read. Close/reopen Hub or reopen the song: the selected file path should be restored from plug-in state.

The plug-in reads `metainfo.xml`, `notes.txt` and `Song/song.xml` inside `.song`; it does not modify the archive, rename clips, or display unsaved changes. The `pos`/`len` values are raw saved time units, not seconds. This first snapshot covers main-timeline `MediaTrack` events in the supplied Studio Pro 8.1.2 sample; takes, layers and unusual event structures need later verification. If a different project is opened in the DAW, choose its `.song` in Hub too.

# Stage 0 — Studio Pro ARA Inspector 0.0a

This is a diagnostic VST3, not the Arranger Manager dashboard. It passes audio through and does not change events.

## Setup

1. Download `Arranger-Manager-Inspector-0.0a-Windows` from the successful GitHub Actions build. Copy `Arranger Manager Inspector.vst3` into your Windows VST3 location.
2. In a **copy** of a Studio Pro project, create three audio tracks named `ARRANGER — Brass`, `ARRANGER — Strings`, `ARRANGER — Bass`.
3. Put actual silent audio events on each track (not empty MIDI Parts). Give them distinct event names, positions and durations. Add the Inspector as an ARA/Event FX to **each service event**. Copying or splitting an event copies its Event FX in the observed Studio Pro session; verify the new event still has it.
4. Open its editor and use **Copy report** after each change. Record what Studio Pro shows on the timeline alongside the report. Times in the report are seconds, not bars.

## Experiments

| Check | Action | Observe |
| --- | --- | --- |
| Baseline | Open Inspector on one of the marked events | `ARA bound`, document controllers, context/sequence/region counts |
| Track scope | Compare three service tracks | Are all three sequences visible in one controller? Are names distinct? |
| Event identity | Rename event, then source file separately | `explicit name` versus `effective name`; can event name be trusted? |
| Position and length | Move and resize event | Start/duration and revision change without reopening UI |
| Color | Recolor event and track separately | Explicit region and sequence RGB; `<not provided>` is a valid finding |
| Structure | Copy/duplicate, split, overlap, then delete events | Event FX copied; region counts and rows match the timeline; changes appear live |
| Disabled FX | Disable the Event FX and edit event names/positions | Does ARA continue to update without audio processing? |
| Mute | Mute an event and then a track | Does ARA still expose the same regions? |
| Persistence | Save, close, reopen project | Stable names/positions and ARA visibility |

The report shows what **this ARA controller** sees; a count of one is not evidence that the entire DAW project has only one event. Test an event without the Inspector separately before assuming it is included in the ARA graph.

## Observed in Studio Pro (2026-09-27)

- Eight events with eight Event FX on two audio tracks appeared as **two sequences / eight regions in one document controller**.
- Explicit event names and region/sequence colors were available. Renaming two events and moving another event from 120 s to 128 s updated the open report; revision advanced from 111 to 122.
- The user observed that copy/split carries the Event FX, and that disabled Event FX still receives ARA changes in the open session.
- The user reports that changes remain stable after saving/reopening and with mute. The screenshot's Performance Monitor also shows all eight Event FX copies disabled while the open report reflects the edits.
- The screenshots use audible events with waveforms. An actual silent audio source/event must still be tested before treating it as a reliable Arrangement Map marker.
- Resize, recolor update, deletion, overlap, and events lacking the ARA insert have not been demonstrated individually. Do not treat the advancing revision alone as proof of every operation.

## Pass criteria

- Reliable start/duration for distinct service events.
- Reliable identification of logical category/status through the explicit region name or another demonstrated field.
- Live move/resize/rename updates and clear track scope.
- Color is optional; absent color must not block Stage 1.

Send the reports and screenshots of the remaining tests. Current evidence favors a shared document-level reader with the ARA Event FX present on each marked event; persistence and edge cases remain open.

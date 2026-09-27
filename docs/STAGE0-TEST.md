# Stage 0 — Studio Pro ARA Inspector 0.0a

This is a diagnostic VST3, not the Arranger Manager dashboard. It passes audio through and does not change events.

## Setup

1. Download `Arranger-Manager-Inspector-0.0a-Windows` from the successful GitHub Actions build. Copy `Arranger Manager Inspector.vst3` into your Windows VST3 location.
2. In a **copy** of a Studio Pro project, create three audio tracks named `ARRANGER — Brass`, `ARRANGER — Strings`, `ARRANGER — Bass`.
3. Put actual silent audio events on each track (not empty MIDI Parts). Give them distinct event names, positions and durations. Add the Inspector as an ARA/Event FX to one of the silent events, as with Smart Voicing ARA.
4. Open its editor and use **Copy report** after each change. Record what Studio Pro shows on the timeline alongside the report. Times in the report are seconds, not bars.

## Experiments

| Check | Action | Observe |
| --- | --- | --- |
| Baseline | Open Inspector on one silent event | `ARA bound`, document controllers, context/sequence/region counts |
| Track scope | Compare three service tracks | Are all three sequences visible in one controller? Are names distinct? |
| Event identity | Rename event, then source file separately | `explicit name` versus `effective name`; can event name be trusted? |
| Position and length | Move and resize event | Start/duration and revision change without reopening UI |
| Color | Recolor event and track separately | Explicit region and sequence RGB; `<not provided>` is a valid finding |
| Structure | Copy/duplicate, split, overlap, then delete events | Region counts and rows match the timeline; changes appear live |
| Mute | Mute an event and then a track | Does ARA still expose the same regions? |
| Persistence | Save, close, reopen project | Stable names/positions and ARA visibility |

If the Inspector shows only the event to which it was attached, try one instance per service track and compare each report. The report shows what **this ARA controller** sees; a count of one is not evidence that the entire DAW project has only one event.

## Pass criteria

- Reliable start/duration for distinct service events.
- Reliable identification of logical category/status through the explicit region name or another demonstrated field.
- Live move/resize/rename updates and clear track scope.
- Color is optional; absent color must not block Stage 1.

Send the reports and screenshots of the matching timeline. We will decide between a central reader, readers per track, or plugin-owned metadata based on those observations.

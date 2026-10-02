# Map Preview 0.0b — statuses and priorities

This prototype reads the existing ARA event names. It does not rename or recolor Studio Pro events. Put the ARA Event FX on each marked audio event. Use a real silent WAV source and mute the service track if desired; keep the events themselves unmuted to retain their colors.

## Event name

`STATUS | P1 | optional short note`

Examples:

- `WIP | P1 | transition into chorus`
- `POOL | P2 | sketch to sort later`
- `DRAFT | P2 | bass groove`
- `REVIEW | P1 | check dynamics`
- `DONE | P2`
- `BLOCKED | P0 | waiting for melody`

The track names the instrument group and the timeline locates the section. The event name only needs status, priority and a short task. Codes are ASCII and case-insensitive; a note may use Cyrillic. Existing `BRASS:DONE` and plain `DONE` names are accepted with no priority. Untagged DAW events appear as `UNMARKED` in the preview and are not valid arrangement tags.

## Color contract

| Status | Meaning | Event color and preview swatch |
| --- | --- | --- |
| `POOL` | Idea or unassigned material | Violet `#8B78B8` |
| `TODO` | Not started | Gray `#818A96` |
| `WIP` | Actively being worked on | Blue `#3B82F6` |
| `DRAFT` | First pass exists, needs polish | Cyan `#35B8D6` |
| `REVIEW` | Ready to check/listen | Amber `#E9B949` |
| `DONE` | Accepted | Green `#34A878` |
| `BLOCKED` | Waiting on a decision/dependency | Red `#DD5B61` |

| Priority | Meaning | Preview badge |
| --- | --- | --- |
| `P0` | Urgent blocker | Red `#DD5B61` |
| `P1` | High | Orange `#ED9844` |
| `P2` | Normal | Blue `#679FE8` |
| `P3` | Low | Gray `#929AAA` |

Studio Pro gives an event one color. Use its event color for **status** (choose the closest host palette color); the priority remains in the name and receives a separate badge inside the plug-in. The name is authoritative if manual DAW coloring differs. The Inspector keeps its raw ARA report below the new preview.

## Current scope

This build is a visual parser preview. It does not calculate coverage, sort Attention, create tasks, edit event properties, or change the DAW event color. The next stage can use the parsed statuses and priorities as inputs to the Arrangement Map.

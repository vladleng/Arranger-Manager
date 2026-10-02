#pragma once

#include "SongSnapshot.h"

namespace arranger
{
inline juce::String noteField(const juce::String& value)
{
    return juce::String(value.length()) + ":" + value;
}

inline juce::String trackNoteKey(const SongTrack& track, size_t trackIndex)
{
    return "track:" + noteField(track.id.isNotEmpty() ? track.id
        : "index:" + juce::String(static_cast<int>(trackIndex)));
}

inline juce::String clipNoteBaseKey(const juce::String& trackKey, const SongEvent& event)
{
    // A clipID can recur after split/copy; position, duration and type distinguish the events.
    // Names are intentionally excluded so status/title edits do not detach local notes.
    return "clip:" + noteField(trackKey) + noteField(event.type) + noteField(event.clipId)
        + noteField(event.start) + noteField(event.length) + noteField(event.timeFormat);
}

inline juce::String clipNoteKey(const juce::String& trackKey, const SongEvent& event,
    size_t occurrence)
{
    return clipNoteBaseKey(trackKey, event) + "#" + juce::String(static_cast<int>(occurrence));
}

inline juce::String taskNoteKey(const juce::String& id) { return "task:" + noteField(id); }
inline juce::String checkpointNoteKey(const juce::String& id) { return "checkpoint:" + noteField(id); }
inline juce::String songNoteKey() { return "song"; }
}

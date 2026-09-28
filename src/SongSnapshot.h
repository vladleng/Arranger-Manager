#pragma once
#include <JuceHeader.h>
#include <vector>

namespace arranger
{
struct SongEvent
{
    juce::String track, trackId, mediaType, type, name, start, length, timeFormat;
};

struct SongSnapshot
{
    juce::String documentTitle, mediaTitle, artist, notes, error;
    int trackCount = 0;
    std::vector<SongEvent> events;
    bool ok() const { return error.isEmpty(); }
};

SongSnapshot readSongSnapshot(const juce::File& song);
}

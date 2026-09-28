#pragma once
#include <juce_core/juce_core.h>
#include <vector>

namespace arranger
{
struct SongEvent
{
    juce::String track, trackId, mediaType, type, name, start, length, timeFormat;
};

struct SongTrack
{
    juce::String id, name, mediaType, notes, color;
    std::vector<SongEvent> events;
};

struct TimelineItem
{
    juce::String name, start, length, timeFormat, type, color;
};

struct SongSnapshot
{
    juce::String documentTitle, mediaTitle, artist, notes, error, clipRangeWarning;
    int trackCount = 0;
    std::vector<SongTrack> tracks;
    std::vector<SongEvent> events;
    std::vector<TimelineItem> sections, markers;
    bool ok() const { return error.isEmpty(); }
};

SongSnapshot readSongSnapshot(const juce::File& song);
std::vector<size_t> matchingSectionIndices(const SongSnapshot&, const SongEvent&);
}

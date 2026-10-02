#pragma once
#include <juce_core/juce_core.h>
#include <vector>

namespace arranger
{
struct SongEvent
{
    juce::String track, trackId, mediaType, type, name, start, length, timeFormat, clipId;
};

struct SongTrack
{
    juce::String id, name, mediaType, notes, color, parentFolder;
    std::vector<SongEvent> events;
};

struct SongTrackFolder
{
    juce::String id, name, color, parentFolder;
};

struct SongTrackEntry
{
    bool folder = false;
    size_t index = 0;
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
    std::vector<SongTrackFolder> trackFolders;
    std::vector<SongTrackEntry> trackOrder;
    std::vector<SongEvent> events;
    std::vector<TimelineItem> sections, markers;
    bool ok() const { return error.isEmpty(); }
};

SongSnapshot readSongSnapshot(const juce::File& song);
std::vector<size_t> matchingSectionIndices(const SongSnapshot&, const SongEvent&);
}

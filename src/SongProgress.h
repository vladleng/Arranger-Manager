#pragma once

#include "ArrangementTag.h"
#include "SongSnapshot.h"
#include <utility>

namespace arranger
{
// Untagged audio/MIDI events are ordinary DAW clips, not tasks in progress.
inline std::pair<int, int> clipProgress(const std::vector<SongEvent>& events)
{
    int done = 0, tagged = 0;
    for (const auto& event : events)
    {
        const auto tag = parse(event.name.toStdString());
        if (!tag.valid) continue;
        ++tagged;
        if (tag.status == Status::done) ++done;
    }
    return {done, tagged};
}

inline bool managedTrack(const SongTrack& track)
{
    return parseTrackNote(track.notes.toStdString()).status != Status::unmarked;
}

inline int managedTrackCount(const SongSnapshot& song)
{
    int count = 0;
    for (const auto& track : song.tracks)
        if (managedTrack(track)) ++count;
    return count;
}

inline std::pair<int, int> songProgress(const SongSnapshot& song)
{
    int done = 0, total = 0;
    for (const auto& track : song.tracks)
    {
        if (!managedTrack(track)) continue;
        const auto [d, t] = clipProgress(track.events);
        done += d;
        total += t;
    }
    return {done, total};
}
}

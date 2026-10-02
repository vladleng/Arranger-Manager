#pragma once

#include "ArrangementTag.h"
#include "SongSnapshot.h"
#include "SongCatalog.h"
#include <map>
#include <set>
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
    return parseTrackName(track.name.toStdString()).status != Status::unmarked;
}

inline int managedTrackCount(const SongSnapshot& song)
{
    int count = 0;
    for (const auto& track : song.tracks)
        if (managedTrack(track)) ++count;
    return count;
}

inline bool folderContainsManagedTrack(const SongSnapshot& song, const juce::String& folderId)
{
    if (folderId.isEmpty()) return false;
    std::map<juce::String, juce::String> parents;
    for (const auto& folder : song.trackFolders)
        if (folder.id.isNotEmpty()) parents.try_emplace(folder.id, folder.parentFolder);
    for (const auto& track : song.tracks)
    {
        if (!managedTrack(track)) continue;
        auto parent = track.parentFolder;
        std::set<juce::String> seen;
        while (parent.isNotEmpty() && seen.insert(parent).second)
        {
            if (parent == folderId) return true;
            const auto found = parents.find(parent);
            if (found == parents.end()) break;
            parent = found->second;
        }
    }
    return false;
}

inline std::pair<int, int> songProgress(const SongSnapshot& song)
{
    int done = 0, total = 0;
    for (const auto& track : song.tracks)
    {
        const auto status = parseTrackName(track.name.toStdString()).status;
        if (status == Status::unmarked) continue;
        ++total;
        if (status == Status::done) ++done;
    }
    return {done, total};
}

inline std::pair<int, int> songProgress(const SongSnapshot& song, const CatalogSong* catalogSong)
{
    auto [done, total] = songProgress(song);
    if (catalogSong != nullptr)
        for (const auto& task : catalogSong->tasks)
        {
            ++total;
            if (task.status == Status::done) ++done;
        }
    return {done, total};
}
}

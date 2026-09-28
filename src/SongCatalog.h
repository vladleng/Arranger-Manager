#pragma once

#include <juce_core/juce_core.h>
#include "ArrangementTag.h"
#include <algorithm>
#include <vector>

namespace arranger
{
struct CatalogFolder { juce::String id, name; };
struct CatalogSong { juce::String path, folderId; Status status = Status::unmarked; };
struct CatalogTrackStatus { juce::String path, trackId; Status status = Status::unmarked; };

class SongCatalog
{
public:
    std::vector<CatalogFolder> folders;
    std::vector<CatalogSong> songs;
    std::vector<CatalogTrackStatus> trackStatuses;

    Status songStatus(const juce::String& path) const
    {
        for (const auto& song : songs)
            if (song.path.equalsIgnoreCase(path)) return song.status;
        return Status::unmarked;
    }

    bool setSongStatus(const juce::String& path, Status status)
    {
        for (auto& song : songs)
            if (song.path.equalsIgnoreCase(path)) { song.status = status; return true; }
        return false;
    }

    Status trackStatus(const juce::String& path, const juce::String& trackId) const
    {
        for (const auto& track : trackStatuses)
            if (track.path.equalsIgnoreCase(path) && track.trackId == trackId) return track.status;
        return Status::unmarked;
    }

    bool setTrackStatus(const juce::String& path, const juce::String& trackId, Status status)
    {
        if (trackId.isEmpty()) return false;
        bool found = false;
        for (const auto& song : songs)
            if (song.path.equalsIgnoreCase(path)) { found = true; break; }
        if (!found) return false;
        for (auto it = trackStatuses.begin(); it != trackStatuses.end(); ++it)
            if (it->path.equalsIgnoreCase(path) && it->trackId == trackId)
            {
                if (status == Status::unmarked) trackStatuses.erase(it);
                else it->status = status;
                return true;
            }
        if (status != Status::unmarked) trackStatuses.push_back({path, trackId, status});
        return true;
    }

    juce::String addFolder(juce::String name)
    {
        name = name.trim();
        if (name.isEmpty()) return {};
        for (const auto& folder : folders)
            if (folder.name.equalsIgnoreCase(name)) return {};
        const auto id = juce::Uuid().toString();
        folders.push_back({id, name});
        return id;
    }

    bool addSong(juce::String path, const juce::String& folderId = {})
    {
        if (path.isEmpty() || !juce::File(path).hasFileExtension("song") || !validFolder(folderId)) return false;
        for (const auto& song : songs)
            if (song.path.equalsIgnoreCase(path)) return false;
        songs.push_back({std::move(path), folderId});
        return true;
    }

    bool moveSong(const juce::String& path, const juce::String& folderId)
    {
        if (!validFolder(folderId)) return false;
        for (auto& song : songs)
            if (song.path.equalsIgnoreCase(path)) { song.folderId = folderId; return true; }
        return false;
    }

    bool removeSong(const juce::String& path)
    {
        for (auto it = songs.begin(); it != songs.end(); ++it)
            if (it->path.equalsIgnoreCase(path))
            {
                songs.erase(it);
                trackStatuses.erase(std::remove_if(trackStatuses.begin(), trackStatuses.end(),
                    [&](const auto& track) { return track.path.equalsIgnoreCase(path); }), trackStatuses.end());
                return true;
            }
        return false;
    }

    bool removeFolder(const juce::String& id)
    {
        for (auto it = folders.begin(); it != folders.end(); ++it)
            if (it->id == id)
            {
                folders.erase(it);
                for (auto& song : songs)
                    if (song.folderId == id) song.folderId.clear();
                return true;
            }
        return false;
    }

    juce::String toJson() const
    {
        auto root = std::make_unique<juce::DynamicObject>();
        root->setProperty("version", 1);
        juce::Array<juce::var> folderArray, songArray, trackArray;
        for (const auto& folder : folders)
        {
            auto item = std::make_unique<juce::DynamicObject>();
            item->setProperty("id", folder.id);
            item->setProperty("name", folder.name);
            folderArray.add(juce::var(item.release()));
        }
        for (const auto& song : songs)
        {
            auto item = std::make_unique<juce::DynamicObject>();
            item->setProperty("path", song.path);
            item->setProperty("folderId", song.folderId);
            item->setProperty("status", label(song.status));
            songArray.add(juce::var(item.release()));
        }
        for (const auto& track : trackStatuses)
        {
            auto item = std::make_unique<juce::DynamicObject>();
            item->setProperty("path", track.path);
            item->setProperty("trackId", track.trackId);
            item->setProperty("status", label(track.status));
            trackArray.add(juce::var(item.release()));
        }
        root->setProperty("folders", folderArray);
        root->setProperty("songs", songArray);
        root->setProperty("trackStatuses", trackArray);
        return juce::JSON::toString(juce::var(root.release()));
    }

    static SongCatalog fromJson(const juce::String& text)
    {
        SongCatalog catalog;
        const auto root = juce::JSON::parse(text);
        if (!root.isObject() || static_cast<int>(root.getProperty("version", 0)) != 1) return catalog;
        const auto folderList = root.getProperty("folders", {});
        if (const auto* array = folderList.getArray())
            for (const auto& item : *array)
            {
                if (!item.isObject()) continue;
                const auto id = item.getProperty("id", {}).toString();
                const auto name = item.getProperty("name", {}).toString().trim();
                if (id.isNotEmpty() && name.isNotEmpty() && !catalog.validFolder(id))
                    catalog.folders.push_back({id, name});
            }
        const auto songList = root.getProperty("songs", {});
        if (const auto* array = songList.getArray())
            for (const auto& item : *array)
            {
                if (!item.isObject()) continue;
                const auto path = item.getProperty("path", {}).toString();
                auto folderId = item.getProperty("folderId", {}).toString();
                if (!catalog.validFolder(folderId)) folderId.clear();
                if (catalog.addSong(path, folderId))
                    catalog.setSongStatus(path, parseStatus(item.getProperty("status", {}).toString().toStdString()));
            }
        const auto trackList = root.getProperty("trackStatuses", {});
        if (const auto* array = trackList.getArray())
            for (const auto& item : *array)
                if (item.isObject())
                    catalog.setTrackStatus(item.getProperty("path", {}).toString(),
                        item.getProperty("trackId", {}).toString(),
                        parseStatus(item.getProperty("status", {}).toString().toStdString()));
        return catalog;
    }

private:
    bool validFolder(const juce::String& id) const
    {
        if (id.isEmpty()) return true;
        for (const auto& folder : folders)
            if (folder.id == id) return true;
        return false;
    }
};
}

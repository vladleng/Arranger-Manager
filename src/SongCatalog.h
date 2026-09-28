#pragma once

#include <juce_core/juce_core.h>
#include <vector>

namespace arranger
{
struct CatalogFolder { juce::String id, name; };
struct CatalogSong { juce::String path, folderId; };

class SongCatalog
{
public:
    std::vector<CatalogFolder> folders;
    std::vector<CatalogSong> songs;

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
            if (it->path.equalsIgnoreCase(path)) { songs.erase(it); return true; }
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
        juce::Array<juce::var> folderArray, songArray;
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
            songArray.add(juce::var(item.release()));
        }
        root->setProperty("folders", folderArray);
        root->setProperty("songs", songArray);
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
                catalog.addSong(path, folderId);
            }
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

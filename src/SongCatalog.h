#pragma once

#include <juce_core/juce_core.h>
#include "ArrangementTag.h"
#include "SongNoteKeys.h"
#include <algorithm>
#include <utility>
#include <vector>

namespace arranger
{
enum class SongType { unspecified, beginning, rough, mixing, finalMix };
inline const char* songTypeKey(SongType type)
{
    switch (type)
    {
        case SongType::beginning: return "beginning";
        case SongType::rough: return "rough";
        case SongType::mixing: return "mixing";
        case SongType::finalMix: return "finalMix";
        default: return "";
    }
}
inline SongType parseSongType(const juce::String& key)
{
    for (auto type : {SongType::beginning, SongType::rough, SongType::mixing, SongType::finalMix})
        if (key == songTypeKey(type)) return type;
    return SongType::unspecified;
}

struct CatalogFolder { juce::String id, name; };
struct CatalogNote { juce::String key, text; };
struct CatalogCheckpoint { juce::String id, name; Status status = Status::todo; };
struct CatalogTask
{
    juce::String id, name;
    Status status = Status::todo;
    std::vector<CatalogCheckpoint> checkpoints;

    std::pair<int, int> progress() const
    {
        int done = 0;
        for (const auto& checkpoint : checkpoints)
            if (checkpoint.status == Status::done) ++done;
        return {done, static_cast<int>(checkpoints.size())};
    }
};
struct CatalogSong
{
    juce::String path, folderId;
    Status status = Status::unmarked;
    std::vector<CatalogTask> tasks;
    std::vector<CatalogNote> localNotes;
    SongType type = SongType::unspecified;
    juce::String id, pageId; // Identity survives path changes (workspace schema 1).
};

class SongCatalog
{
public:
    std::vector<CatalogFolder> folders;
    std::vector<CatalogSong> songs;

    // Existing 0.1 catalog JSON can contain trackStatuses. They are ignored:
    // the Studio Pro track name is now the sole source of track status.

    std::pair<int, int> folderProgress(const juce::String& folderId) const
    {
        int done = 0, total = 0;
        for (const auto& song : songs)
            if (song.folderId == folderId)
            {
                ++total;
                if (song.status == Status::done) ++done;
            }
        return {done, total};
    }

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

    bool setSongType(const juce::String& path, SongType type)
    {
        if (auto* song = findSong(path)) { song->type = type; return true; }
        return false;
    }

    template <typename Item>
    static bool reorder(std::vector<Item>& items, const juce::String& sourceId,
        const juce::String& targetId, bool after)
    {
        auto source = std::find_if(items.begin(), items.end(), [&](const auto& item) { return item.id == sourceId; });
        auto target = std::find_if(items.begin(), items.end(), [&](const auto& item) { return item.id == targetId; });
        if (source == items.end() || target == items.end() || source == target) return false;
        const auto from = static_cast<size_t>(source - items.begin());
        auto to = static_cast<size_t>(target - items.begin()) + (after ? 1u : 0u);
        if (to > from) --to;
        if (to == from) return false;
        auto moved = std::move(items[from]);
        items.erase(items.begin() + static_cast<std::ptrdiff_t>(from));
        items.insert(items.begin() + static_cast<std::ptrdiff_t>(to), std::move(moved));
        return true;
    }

    bool moveTask(const juce::String& path, const juce::String& id,
        const juce::String& targetId, bool after)
    {
        if (auto* song = findSong(path)) return reorder(song->tasks, id, targetId, after);
        return false;
    }

    bool moveCheckpoint(const juce::String& path, const juce::String& taskId,
        const juce::String& id, const juce::String& targetId, bool after)
    {
        if (auto* task = findTask(path, taskId)) return reorder(task->checkpoints, id, targetId, after);
        return false;
    }

    juce::String localNote(const juce::String& path, const juce::String& key) const
    {
        if (const auto* song = findSong(path))
            for (const auto& note : song->localNotes)
                if (note.key == key) return note.text;
        return {};
    }

    bool setLocalNote(const juce::String& path, const juce::String& key, juce::String text)
    {
        auto* song = findSong(path);
        if (song == nullptr || key.isEmpty()) return false;
        text = text.trim();
        for (auto it = song->localNotes.begin(); it != song->localNotes.end(); ++it)
            if (it->key == key)
            {
                if (text.isEmpty()) song->localNotes.erase(it);
                else it->text = std::move(text);
                return true;
            }
        if (text.isNotEmpty()) song->localNotes.push_back({key, std::move(text)});
        return true;
    }

    const CatalogSong* findSong(const juce::String& path) const
    {
        for (const auto& song : songs)
            if (song.path.equalsIgnoreCase(path)) return &song;
        return nullptr;
    }

    CatalogSong* findSong(const juce::String& path)
    {
        return const_cast<CatalogSong*>(std::as_const(*this).findSong(path));
    }

    CatalogTask* findTask(const juce::String& path, const juce::String& id)
    {
        if (auto* song = findSong(path))
            for (auto& task : song->tasks)
                if (task.id == id) return &task;
        return nullptr;
    }

    CatalogSong* findSongById(const juce::String& id)
    {
        for (auto& song : songs) if (song.id == id) return &song;
        return nullptr;
    }

    bool relinkSong(const juce::String& id, const juce::String& newPath)
    {
        auto* song = findSongById(id);
        if (song == nullptr || newPath.isEmpty() || !juce::File(newPath).hasFileExtension("song")) return false;
        if (const auto* existing = findSong(newPath); existing != nullptr && existing != song) return false;
        song->path = newPath;
        return true;
    }

    juce::String addTask(const juce::String& path, juce::String name)
    {
        name = name.trim();
        auto* song = findSong(path);
        if (song == nullptr || name.isEmpty()) return {};
        const auto id = juce::Uuid().toString();
        song->tasks.push_back({id, name});
        return id;
    }

    juce::String addCheckpoint(const juce::String& path, const juce::String& taskId, juce::String name)
    {
        name = name.trim();
        auto* task = findTask(path, taskId);
        if (task == nullptr || name.isEmpty()) return {};
        const auto id = juce::Uuid().toString();
        task->checkpoints.push_back({id, name});
        return id;
    }

    bool editTask(const juce::String& path, const juce::String& id, juce::String name)
    {
        if (auto* task = findTask(path, id); task != nullptr && name.trim().isNotEmpty())
        { task->name = name.trim(); return true; }
        return false;
    }

    bool editCheckpoint(const juce::String& path, const juce::String& taskId,
        const juce::String& id, juce::String name)
    {
        if (auto* task = findTask(path, taskId); task != nullptr && name.trim().isNotEmpty())
            for (auto& item : task->checkpoints)
                if (item.id == id) { item.name = name.trim(); return true; }
        return false;
    }

    bool setTaskStatus(const juce::String& path, const juce::String& id, Status status)
    {
        if (auto* task = findTask(path, id)) { task->status = status; return true; }
        return false;
    }

    bool setCheckpointStatus(const juce::String& path, const juce::String& taskId,
        const juce::String& id, Status status)
    {
        if (auto* task = findTask(path, taskId))
            for (auto& item : task->checkpoints)
                if (item.id == id) { item.status = status; return true; }
        return false;
    }

    bool removeTask(const juce::String& path, const juce::String& id)
    {
        if (auto* song = findSong(path))
            for (auto it = song->tasks.begin(); it != song->tasks.end(); ++it)
                if (it->id == id)
                {
                    setLocalNote(path, taskNoteKey(id), {});
                    for (const auto& cp : it->checkpoints)
                        setLocalNote(path, checkpointNoteKey(cp.id), {});
                    song->tasks.erase(it);
                    return true;
                }
        return false;
    }

    bool removeCheckpoint(const juce::String& path, const juce::String& taskId, const juce::String& id)
    {
        if (auto* task = findTask(path, taskId))
            for (auto it = task->checkpoints.begin(); it != task->checkpoints.end(); ++it)
                if (it->id == id)
                {
                    setLocalNote(path, checkpointNoteKey(id), {});
                    task->checkpoints.erase(it);
                    return true;
                }
        return false;
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
        songs.back().id = juce::Uuid().toString();
        songs.back().pageId = juce::Uuid().toString();
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
        root->setProperty("version", 2);
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
            item->setProperty("id", song.id);
            item->setProperty("pageId", song.pageId);
            item->setProperty("path", song.path);
            item->setProperty("folderId", song.folderId);
            item->setProperty("status", label(song.status));
            item->setProperty("songType", songTypeKey(song.type));
            juce::Array<juce::var> taskArray;
            for (const auto& task : song.tasks)
            {
                auto taskItem = std::make_unique<juce::DynamicObject>();
                taskItem->setProperty("id", task.id);
                taskItem->setProperty("name", task.name);
                taskItem->setProperty("status", label(task.status));
                juce::Array<juce::var> checkpointArray;
                for (const auto& checkpoint : task.checkpoints)
                {
                    auto cp = std::make_unique<juce::DynamicObject>();
                    cp->setProperty("id", checkpoint.id);
                    cp->setProperty("name", checkpoint.name);
                    cp->setProperty("status", label(checkpoint.status));
                    checkpointArray.add(juce::var(cp.release()));
                }
                taskItem->setProperty("checkpoints", checkpointArray);
                taskArray.add(juce::var(taskItem.release()));
            }
            item->setProperty("tasks", taskArray);
            juce::Array<juce::var> notesArray;
            for (const auto& note : song.localNotes)
            {
                auto entry = std::make_unique<juce::DynamicObject>();
                entry->setProperty("key", note.key);
                entry->setProperty("text", note.text);
                notesArray.add(juce::var(entry.release()));
            }
            item->setProperty("localNotes", notesArray);
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
        const auto version = static_cast<int>(root.getProperty("version", 0));
        if (!root.isObject() || (version != 1 && version != 2)) return catalog;
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
                {
                    if (version == 2)
                    {
                        catalog.songs.back().id = item.getProperty("id", {}).toString();
                        catalog.songs.back().pageId = item.getProperty("pageId", {}).toString();
                    }
                    catalog.setSongStatus(path, parseStatus(item.getProperty("status", {}).toString().toStdString()));
                    catalog.setSongType(path, parseSongType(item.getProperty("songType", {}).toString()));
                    const auto notesList = item.getProperty("localNotes", {});
                    if (const auto* entries = notesList.getArray())
                        for (const auto& entry : *entries)
                            if (entry.isObject())
                            {
                                const auto key = entry.getProperty("key", {}).toString();
                                const auto value = entry.getProperty("text", {}).toString();
                                if (catalog.localNote(path, key).isEmpty())
                                    catalog.setLocalNote(path, key, value);
                            }
                    const auto taskList = item.getProperty("tasks", {});
                    if (const auto* tasks = taskList.getArray())
                        for (const auto& task : *tasks)
                        {
                            if (!task.isObject()) continue;
                            const auto name = task.getProperty("name", {}).toString();
                            const auto id = task.getProperty("id", {}).toString();
                            if (name.trim().isEmpty() || id.isEmpty() || catalog.findTask(path, id)) continue;
                            auto* song = catalog.findSong(path);
                            song->tasks.push_back({id, name, parseStatus(task.getProperty("status", {}).toString().toStdString())});
                            auto& added = song->tasks.back();
                            const auto checkpointList = task.getProperty("checkpoints", {});
                            if (const auto* checkpoints = checkpointList.getArray())
                                for (const auto& cp : *checkpoints)
                                {
                                    if (!cp.isObject()) continue;
                                    const auto cpName = cp.getProperty("name", {}).toString();
                                    const auto cpId = cp.getProperty("id", {}).toString();
                                    if (cpName.trim().isEmpty() || cpId.isEmpty()) continue;
                                    bool duplicate = false;
                                    for (const auto& old : added.checkpoints)
                                        if (old.id == cpId) duplicate = true;
                                    if (!duplicate) added.checkpoints.push_back({cpId, cpName,
                                        parseStatus(cp.getProperty("status", {}).toString().toStdString())});
                                }
                        }
                }
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

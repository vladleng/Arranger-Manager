#pragma once

#include <juce_core/juce_core.h>
#include "ArrangementTag.h"
#include <utility>
#include <vector>

namespace arranger
{
struct CatalogFolder { juce::String id, name; };
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
                if (it->id == id) { song->tasks.erase(it); return true; }
        return false;
    }

    bool removeCheckpoint(const juce::String& path, const juce::String& taskId, const juce::String& id)
    {
        if (auto* task = findTask(path, taskId))
            for (auto it = task->checkpoints.begin(); it != task->checkpoints.end(); ++it)
                if (it->id == id) { task->checkpoints.erase(it); return true; }
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
            item->setProperty("status", label(song.status));
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
                if (catalog.addSong(path, folderId))
                {
                    catalog.setSongStatus(path, parseStatus(item.getProperty("status", {}).toString().toStdString()));
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

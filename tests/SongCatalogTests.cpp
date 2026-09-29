#include "SongCatalog.h"
#include "SongProgress.h"
#include <iostream>

bool check(bool condition, const char* message)
{
    if (!condition) std::cerr << message << '\n';
    return condition;
}

int main()
{
    arranger::SongCatalog catalog;
    const auto album = catalog.addFolder("Album");
    const auto live = catalog.addFolder("Live");
    if (!check(album.isNotEmpty() && live.isNotEmpty() && album != live, "Folder creation failed")
        || !check(catalog.addFolder("album").isEmpty(), "Duplicate folder accepted")
        || !check(catalog.addSong("C:\\Songs\\First.song", album), "Song import failed")
        || !check(catalog.addSong("C:\\Songs\\Second.song"), "Root song import failed")
        || !check(!catalog.addSong("c:\\songs\\first.song"), "Duplicate song accepted")) return 1;

    if (!check(catalog.setSongStatus("C:\\Songs\\First.song", arranger::Status::wait), "Song status assignment failed")
        || !check(catalog.folderProgress(album) == std::pair<int, int>{0, 1}, "Folder WAIT progress failed")
        || !check(catalog.setSongStatus("C:\\Songs\\First.song", arranger::Status::done), "Song DONE assignment failed")
        || !check(catalog.folderProgress(album) == std::pair<int, int>{1, 1}, "Folder DONE progress failed")
        || !check(catalog.folderProgress({}) == std::pair<int, int>{0, 1}, "Unmarked song must remain unfinished")) return 1;

    auto restored = arranger::SongCatalog::fromJson(catalog.toJson());
    if (!check(restored.folders.size() == 2 && restored.songs.size() == 2, "Catalog round-trip failed")
        || !check(restored.songStatus("c:\\songs\\first.song") == arranger::Status::done,
            "Song status lost on restart")
        || !check(restored.songs[0].folderId == album && restored.songs[1].folderId.isEmpty(), "Folder assignment lost")
        || !check(restored.moveSong("C:\\Songs\\Second.song", live), "Song move failed")
        || !check(restored.removeFolder(live) && restored.songs[1].folderId.isEmpty(), "Folder removal lost song")
        || !check(restored.removeSong("C:\\Songs\\First.song") && restored.songs.size() == 1,
            "Song removal failed")) return 1;
    const auto oldCatalog = arranger::SongCatalog::fromJson(
        R"({"version":1,"folders":[],"songs":[{"path":"C:\\Songs\\Legacy.song","folderId":""}]})");
    if (!check(oldCatalog.songs.size() == 1 && oldCatalog.songStatus(oldCatalog.songs[0].path)
        == arranger::Status::unmarked, "Existing 0.0m catalog did not load")
        || !check(oldCatalog.songs[0].type == arranger::SongType::unspecified,
            "Legacy song unexpectedly gained a type")) return 1;
    const auto previousCatalog = arranger::SongCatalog::fromJson(
        R"({"version":1,"folders":[],"songs":[{"path":"C:\\Songs\\Legacy.song","folderId":"","status":"WAIT"}],"trackStatuses":[{"path":"C:\\Songs\\Legacy.song","trackId":"old-track","status":"WIP"}]})");
    if (!check(previousCatalog.songs.size() == 1 && previousCatalog.songStatus(previousCatalog.songs[0].path)
            == arranger::Status::wait, "0.1 song status migration failed")
        || !check(!previousCatalog.toJson().contains("trackStatuses"), "Local track statuses must be ignored")) return 1;

    const auto first = catalog.addTask("C:\\Songs\\First.song", "Composition");
    const auto second = catalog.addTask("C:\\Songs\\First.song", "Harmony");
    const auto verse = catalog.addCheckpoint("C:\\Songs\\First.song", first, "Verse");
    const auto chorus = catalog.addCheckpoint("C:\\Songs\\First.song", first, "Chorus");
    if (!check(first.isNotEmpty() && second.isNotEmpty() && verse.isNotEmpty() && chorus.isNotEmpty(), "Task creation failed")
        || !check(catalog.setCheckpointStatus("C:\\Songs\\First.song", first, verse, arranger::Status::done), "Checkpoint status failed")
        || !check(catalog.findTask("C:\\Songs\\First.song", first)->progress() == std::pair<int, int>{1, 2}, "Task progress failed")) return 1;

    const auto third = catalog.addTask("C:\\Songs\\First.song", "Instrumentation");
    const auto bridge = catalog.addCheckpoint("C:\\Songs\\First.song", first, "Bridge");
    const auto otherCheckpoint = catalog.addCheckpoint("C:\\Songs\\First.song", second, "Unrelated");
    if (!check(catalog.moveTask("C:\\Songs\\First.song", third, first, false), "Move task to top failed")
        || !check(catalog.findSong("C:\\Songs\\First.song")->tasks[0].id == third, "Task order not updated")
        || !check(catalog.moveTask("C:\\Songs\\First.song", third, second, true), "Move task to bottom failed")
        || !check(catalog.findSong("C:\\Songs\\First.song")->tasks[2].id == third, "Task moved to wrong position")
        || !check(!catalog.moveTask("C:\\Songs\\First.song", first, second, false), "No-op task move changed order")
        || !check(!catalog.moveTask("C:\\Songs\\Second.song", first, second, true), "Cross-song task move accepted")
        || !check(catalog.moveCheckpoint("C:\\Songs\\First.song", first, bridge, verse, false), "Checkpoint move failed")
        || !check(catalog.findTask("C:\\Songs\\First.song", first)->checkpoints[0].id == bridge,
            "Checkpoint order not updated")
        || !check(!catalog.moveCheckpoint("C:\\Songs\\First.song", first, bridge, otherCheckpoint, true),
            "Cross-task checkpoint move accepted")
        || !check(catalog.setSongType("C:\\Songs\\First.song", arranger::SongType::rough), "Song type failed")) return 1;

    arranger::SongSnapshot snapshot;
    arranger::SongTrack doneTrack; doneTrack.name = "DONE | Drums";
    arranger::SongTrack workTrack; workTrack.name = "WIP | Bass";
    snapshot.tracks = {doneTrack, workTrack};
    if (!check(arranger::songProgress(snapshot, catalog.findSong("C:\\Songs\\First.song")) == std::pair<int, int>{1, 5},
            "Song progress must count top-level tasks and managed tracks only")
        || !check(catalog.setTaskStatus("C:\\Songs\\First.song", first, arranger::Status::done), "Task status failed")
        || !check(arranger::songProgress(snapshot, catalog.findSong("C:\\Songs\\First.song")) == std::pair<int, int>{2, 5},
            "Only task DONE may raise song progress")) return 1;

    const auto songPath = juce::String("C:\\Songs\\First.song");
    const auto taskKey = arranger::taskNoteKey(first);
    const auto cpKey = arranger::checkpointNoteKey(verse);
    arranger::SongTrack noteTrack; noteTrack.id = "track-1";
    const auto trackKey = arranger::trackNoteKey(noteTrack, 0);
    arranger::SongEvent clip; clip.type = "AudioEvent"; clip.clipId = "clip-1";
    clip.start = "32"; clip.length = "8"; clip.timeFormat = "seconds";
    const auto clipKey = arranger::clipNoteKey(trackKey, clip, 0);
    auto renamed = clip; renamed.name = "DONE | New title";
    auto split = clip; split.start = "40";
    if (!check(clipKey == arranger::clipNoteKey(trackKey, renamed, 0), "Clip rename detached local note")
        || !check(clipKey != arranger::clipNoteKey(trackKey, split, 0), "Split clips shared local note")
        || !check(clipKey != arranger::clipNoteKey(trackKey, clip, 1), "Identical clips shared local note")
        || !check(trackKey == arranger::trackNoteKey(noteTrack, 5), "Track note changed with track ordering")) return 1;
    if (!check(catalog.setLocalNote(songPath, taskKey, "Task idea"), "Task note failed")
        || !check(catalog.setLocalNote(songPath, cpKey, "Checkpoint idea"), "Checkpoint note failed")
        || !check(catalog.setLocalNote(songPath, trackKey, "Local track note"), "Track note failed")
        || !check(catalog.setLocalNote(songPath, clipKey, "Local clip note"), "Clip note failed")) return 1;
    const auto otherPath = juce::String("C:\\Songs\\Second.song");
    if (!check(catalog.localNote(otherPath, clipKey).isEmpty(), "Note leaked between songs")
        || !check(catalog.setLocalNote(songPath, trackKey, ""), "Clear note failed")
        || !check(catalog.localNote(songPath, trackKey).isEmpty(), "Cleared note remained")) return 1;
    auto tasksRestored = arranger::SongCatalog::fromJson(catalog.toJson());
    if (!check(tasksRestored.findTask("C:\\Songs\\First.song", first) != nullptr, "Task round-trip failed")
        || !check(tasksRestored.findTask("C:\\Songs\\First.song", first)->progress() == std::pair<int, int>{1, 3},
            "Checkpoint round-trip failed")
        || !check(tasksRestored.findSong(songPath)->type == arranger::SongType::rough,
            "Song type round-trip failed")
        || !check(tasksRestored.findSong(songPath)->tasks[2].id == third
            && tasksRestored.findTask(songPath, first)->checkpoints[0].id == bridge,
            "Dragged order lost on restart")
        || !check(tasksRestored.findSong("C:\\Songs\\Second.song")->tasks.empty(), "Tasks leaked to another song")
        || !check(tasksRestored.localNote(songPath, clipKey) == "Local clip note", "Clip note round-trip failed")
        || !check(tasksRestored.localNote(songPath, taskKey) == "Task idea", "Task note round-trip failed")
        || !check(tasksRestored.editTask("C:\\Songs\\First.song", second, "Instrumentation"), "Task rename failed")
        || !check(tasksRestored.editCheckpoint("C:\\Songs\\First.song", first, chorus, "Final chorus"), "Checkpoint rename failed")
        || !check(tasksRestored.removeCheckpoint("C:\\Songs\\First.song", first, verse), "Checkpoint removal failed")
        || !check(tasksRestored.localNote(songPath, cpKey).isEmpty(), "Removed checkpoint kept its note")
        || !check(tasksRestored.removeTask("C:\\Songs\\First.song", first), "Task removal failed")
        || !check(tasksRestored.localNote(songPath, taskKey).isEmpty(), "Removed task kept its note")
        || !check(tasksRestored.removeSong("C:\\Songs\\First.song") && tasksRestored.findSong("C:\\Songs\\First.song") == nullptr,
            "Song removal must remove local tasks")) return 1;
    return 0;
}

#include "SongCatalog.h"
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
        == arranger::Status::unmarked, "Existing 0.0m catalog did not load")) return 1;
    const auto previousCatalog = arranger::SongCatalog::fromJson(
        R"({"version":1,"folders":[],"songs":[{"path":"C:\\Songs\\Legacy.song","folderId":"","status":"WAIT"}],"trackStatuses":[{"path":"C:\\Songs\\Legacy.song","trackId":"old-track","status":"WIP"}]})");
    if (!check(previousCatalog.songs.size() == 1 && previousCatalog.songStatus(previousCatalog.songs[0].path)
            == arranger::Status::wait, "0.1 song status migration failed")
        || !check(!previousCatalog.toJson().contains("trackStatuses"), "Local track statuses must be ignored")) return 1;
    return 0;
}

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
        || !check(catalog.setTrackStatus("C:\\Songs\\First.song", "track-42", arranger::Status::wip),
            "Track status assignment failed")) return 1;

    auto restored = arranger::SongCatalog::fromJson(catalog.toJson());
    if (!check(restored.folders.size() == 2 && restored.songs.size() == 2, "Catalog round-trip failed")
        || !check(restored.songStatus("c:\\songs\\first.song") == arranger::Status::wait
            && restored.trackStatus("C:\\Songs\\First.song", "track-42") == arranger::Status::wip,
            "Independent statuses lost on restart")
        || !check(restored.songs[0].folderId == album && restored.songs[1].folderId.isEmpty(), "Folder assignment lost")
        || !check(restored.moveSong("C:\\Songs\\Second.song", live), "Song move failed")
        || !check(restored.removeFolder(live) && restored.songs[1].folderId.isEmpty(), "Folder removal lost song")
        || !check(restored.removeSong("C:\\Songs\\First.song") && restored.songs.size() == 1,
            "Song removal failed")
        || !check(restored.trackStatuses.empty(), "Removed song kept track statuses")) return 1;
    const auto oldCatalog = arranger::SongCatalog::fromJson(
        R"({"version":1,"folders":[],"songs":[{"path":"C:\\Songs\\Legacy.song","folderId":""}]})");
    if (!check(oldCatalog.songs.size() == 1 && oldCatalog.songStatus(oldCatalog.songs[0].path)
        == arranger::Status::unmarked, "Existing 0.0m catalog did not load")) return 1;
    return 0;
}

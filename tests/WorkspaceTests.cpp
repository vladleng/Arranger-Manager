#include "WorkspaceStore.h"
#include "SongProgress.h"
#include <iostream>

static bool check(bool condition, const char* message)
{
    if (!condition) std::cerr << message << '\n';
    return condition;
}
struct TestDirectory
{
    juce::File path = juce::File::getSpecialLocation(juce::File::tempDirectory)
        .getChildFile("ArrangerWorkspaceTest-" + juce::Uuid().toString());
    TestDirectory() { path.createDirectory(); }
    ~TestDirectory() { path.deleteRecursively(); }
};

int main()
{
    using namespace arranger;
    const juce::String legacy = R"({
      "version":1,
      "folders":[{"id":"folder-live","name":"Live"}],
      "songs":[
        {"path":"C:\\Songs\\First.song","folderId":"folder-live","status":"WAIT","songType":"rough",
         "tasks":[
           {"id":"task-harmony","name":"Harmony","status":"WIP",
            "checkpoints":[{"id":"cp-verse","name":"Verse","status":"DONE"},
                           {"id":"cp-chorus","name":"Chorus","status":"TODO"}]},
           {"id":"task-mix","name":"Mix","status":"DONE","checkpoints":[]}],
         "localNotes":[{"key":"song","text":"Song idea"},{"key":"task:task-harmony","text":"Task idea"},
                       {"key":"checkpoint:cp-verse","text":"Checkpoint idea"},
                       {"key":"clip:unchanged-key","text":"Clip note"}]},
        {"path":"C:\\Songs\\Second.song","folderId":"","status":"UNMARKED","songType":"","tasks":[],"localNotes":[]}
      ]})";
    WorkspaceModel model;
    if (!check(WorkspaceModel::migrateLegacy(legacy, model).wasOk(), "Migration failed")) return 1;
    const auto songId = model.catalog.songs[0].id;
    const auto songPageId = model.catalog.songs[0].pageId;
    if (!check(songId.isNotEmpty() && songPageId.isNotEmpty(), "Song IDs were not assigned")
        || !check(model.catalog.songs[0].tasks[0].id == "task-harmony", "Task identity changed")
        || !check(model.catalog.songs[0].tasks[0].checkpoints[1].id == "cp-chorus", "Checkpoint order changed")
        || !check(model.catalog.songs[0].localNotes[3].key == "clip:unchanged-key", "Clip Notes key changed")
        || !check(model.catalog.songs[0].type == SongType::rough, "Song type lost")
        || !check(model.findPage(songPageId)->parentPageId == "folder-live", "Song owner page lost")) return 1;

    TestDirectory directory;
    const auto file = directory.path.getChildFile("workspace.json");
    WorkspaceStore store(file);
    if (!check(store.load(legacy, {}, model).wasOk(), "Initial store migration failed")
        || !check(store.legacyFile().loadFileAsString() == legacy, "Exact legacy backup not retained")
        || !check(file.existsAsFile() && store.backupFile().existsAsFile(), "Workspace and backup missing")) return 1;
    const auto persistedSongId = model.catalog.songs[0].id;
    const auto persistedWorkspaceId = model.id;
    const auto initial = file.loadFileAsString();
    WorkspaceModel reopened;
    WorkspaceStore restart(file);
    if (!check(restart.load("invalid stale settings", {}, reopened).wasOk(), "Restart incorrectly remigrated settings")
        || !check(reopened.id == persistedWorkspaceId && reopened.catalog.songs[0].id == persistedSongId,
            "IDs changed after restart")
        || !check(reopened.catalog.songs.size() == 2 && reopened.pages.size() == 3, "Restart duplicated records")
        || !check(store.legacyFile().loadFileAsString() == legacy, "Restart modified legacy backup")) return 1;

    if (!check(reopened.catalog.relinkSong(persistedSongId, "D:\\Moved\\First.song"), "Relink failed")
        || !check(reopened.catalog.songs[0].id == persistedSongId, "Relink changed song ID")
        || !check(!reopened.catalog.relinkSong(persistedSongId, "C:\\Songs\\Second.song"), "Duplicate relink accepted")) return 1;
    reopened.catalog.setLocalNote(reopened.catalog.songs[0].path, "song", juce::String::fromUTF8("Аранжировка — проверить куплет"));
    reopened.pages.push_back({"prototype-page", {}, "Prototype", "page", {}, {
        {"block-heading", "heading", "Heading", {}, false},
        {"block-list", "list", "First\nSecond", {}, false},
        {"block-check", "checklist", "Check", {}, true},
        {"block-link", "link", "Link", "https://example.com", false}}});
    if (!check(restart.save(reopened).wasOk(), "Workspace save failed")
        || !check(reopened.revision == 2, "Revision did not advance")
        || !check(restart.backupFile().loadFileAsString() == initial, "Previous saved state was not backed up")) return 1;
    WorkspaceModel roundTrip;
    if (!check(WorkspaceModel::fromJson(file.loadFileAsString(), roundTrip).wasOk(), "Workspace round-trip failed")
        || !check(roundTrip.catalog.localNote(roundTrip.catalog.songs[0].path, "song")\n            == juce::String::fromUTF8("Аранжировка — проверить куплет"), "UTF-8 Notes lost")\n        || !check(roundTrip.pages.back().blocks[2].checked, "Checklist state lost")
        || !check(roundTrip.pages.back().blocks[3].url == "https://example.com", "Link lost")
        || !check(roundTrip.catalog.songs[0].tasks[1].id == "task-mix", "Task order lost")) return 1;
    SongSnapshot snapshot;
    SongTrack track; track.name = "DONE | Guitar"; snapshot.tracks.push_back(track);
    if (!check(songProgress(snapshot, &roundTrip.catalog.songs[0]) == std::pair<int, int>{2, 3},
        "Migrated song progress changed")) return 1;

    const auto validText = file.loadFileAsString();
    auto broken = roundTrip;
    broken.pages.back().parentPageId = "missing-page";
    if (!check(broken.validate().failed(), "Dangling parent accepted")) return 1;
    broken = roundTrip;
    broken.pages.back().parentPageId = broken.pages.back().id;
    if (!check(broken.validate().failed(), "Page cycle accepted")) return 1;
    broken = roundTrip;
    broken.catalog.songs[1].id = broken.catalog.songs[0].id;
    if (!check(broken.validate().failed(), "Duplicate song IDs accepted")
        || !check(restart.save(broken).failed(), "Invalid model saved")
        || !check(file.loadFileAsString() == validText, "Invalid save replaced current file")) return 1;

    // Files modified externally must not be silently overwritten.
    file.replaceWithText("{broken");
    if (!check(restart.save(roundTrip).failed(), "External corruption was overwritten")
        || !check(file.loadFileAsString() == "{broken", "Corruption error rewrote the source")) return 1;
    WorkspaceStore corrupted(file);
    WorkspaceModel unchanged;
    unchanged.id = "sentinel";
    if (!check(corrupted.load(legacy, {}, unchanged).failed(), "Corrupt workspace fell back to legacy settings")
        || !check(unchanged.id == "sentinel", "Failed load replaced in-memory state")
        || !check(corrupted.restoreBackup().wasOk(), "Backup recovery failed")
        || !check(file.loadFileAsString() == initial, "Recovery restored wrong version")
        || !check(corrupted.load({}, {}, unchanged).wasOk(), "Recovered document did not open")
        || !check(unchanged.catalog.songs[0].id == persistedSongId, "Recovery lost original IDs")) return 1;
    bool preserved = false;
    for (const auto& candidate : directory.path.findChildFiles(juce::File::findFiles, false, "*.before-restore-*.json"))
        if (candidate.loadFileAsString() == "{broken") preserved = true;
    if (!check(preserved, "Damaged file not retained before recovery")) return 1;

    // Unknown schema, duplicate legacy IDs and malformed legacy data fail closed.
    auto future = juce::JSON::parse(initial);
    future.getDynamicObject()->setProperty("schemaVersion", 99);
    file.replaceWithText(juce::JSON::toString(future));
    const auto futureText = file.loadFileAsString();
    WorkspaceStore newer(file);
    if (!check(newer.load({}, {}, unchanged).failed(), "Future schema accepted")
        || !check(file.loadFileAsString() == futureText, "Future schema overwritten")) return 1;
    const auto duplicateLegacy = legacy.replace("task-mix", "task-harmony");
    if (!check(WorkspaceModel::migrateLegacy(duplicateLegacy, unchanged).failed(), "Duplicate legacy task accepted")
        || !check(WorkspaceModel::migrateLegacy("{bad", unchanged).failed(), "Malformed legacy input accepted")) return 1;
    const auto missingId = initial.replace(persistedSongId, "");
    if (!check(WorkspaceModel::fromJson(missingId, unchanged).failed(), "Missing stored song ID regenerated")) return 1;
    WorkspaceStore invalidLegacy(directory.path.getChildFile("invalid.json"));
    if (!check(invalidLegacy.load("{bad", {}, unchanged).failed(), "Invalid legacy store was created")
        || !check(!invalidLegacy.getFile().exists(), "Failed migration created a workspace")) return 1;

    // Simulate a blocked destination: the previous main and backup remain intact.
    const auto blocked = directory.path.getChildFile("blocked.json");
    blocked.createDirectory();
    WorkspaceStore blockedStore(blocked);
    if (!check(blockedStore.load(legacy, {}, unchanged).failed(), "Directory destination accepted")) return 1;
    WorkspaceStore fresh(directory.path.getChildFile("fresh.json"));
    if (!check(fresh.load({}, "C:\\Songs\\Fallback.song", unchanged).wasOk(), "Empty-settings fallback failed")
        || !check(unchanged.catalog.songs.size() == 1, "Last project was not migrated")) return 1;
    const auto safe = directory.path.getChildFile("backup-blocked.json");
    WorkspaceStore blockedBackup(safe);
    if (!check(blockedBackup.load({}, {}, unchanged).wasOk(), "Backup-failure setup failed")) return 1;
    const auto safeText = safe.loadFileAsString();
    blockedBackup.backupFile().deleteFile();
    blockedBackup.backupFile().createDirectory();
    unchanged.name = "Unsaved change";
    if (!check(blockedBackup.save(unchanged).failed(), "Blocked backup did not prevent save")
        || !check(safe.loadFileAsString() == safeText, "Backup failure replaced main document")) return 1;
    return 0;
}

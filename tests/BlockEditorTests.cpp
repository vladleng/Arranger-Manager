#include "BlockEditorSession.h"
#include "WorkspaceController.h"
#include <iostream>

static bool check(bool value, const char* message)
{
    if (!value) std::cerr << message << '\n';
    return value;
}
#define REQUIRE(value, message) do { if (!check((value), message)) return 1; } while (false)
struct TestDirectory
{
    juce::File path = juce::File::getSpecialLocation(juce::File::tempDirectory)
        .getChildFile("ArrangerBlockTest-" + juce::Uuid().toString());
    TestDirectory() { path.createDirectory(); }
    ~TestDirectory() { path.deleteRecursively(); }
};
int main()
{
    using namespace arranger;
    TestDirectory directory;
    WorkspaceStore store(directory.path.getChildFile("workspace.json"));
    WorkspaceModel model;
    model.catalog.addSong("C:\\Songs\\Song.song");
    const auto taskId = model.catalog.addTask(model.catalog.songs[0].path, "Mix");
    model.catalog.addCheckpoint(model.catalog.songs[0].path, taskId, "Drums");
    model.reconcileCatalogPages();
    WorkspaceCommands commands(model);
    juce::String first, second;
    REQUIRE(commands.createPage({}, "First", first).wasOk(), "Create first");
    REQUIRE(commands.createPage({}, "Second", second).wasOk(), "Create second");
    REQUIRE(store.save(model).wasOk(), "Initial save");
    WorkspaceController controller(model, store);
    BlockEditorSession session;
    auto read = [&](juce::String id) -> BlockEditorSession::Read
    {
        return [&, id]() -> std::optional<BlockEditorSession::Blocks>
        {
            const auto* page = model.findPage(id);
            if (!page || WorkspaceQueries::isArchived(model, id)) return {};
            return page->blocks;
        };
    };
    bool rejectSave = false;
    auto write = [&](juce::String id) -> BlockEditorSession::Write
    {
        return [&, id](const auto& expected, const auto& blocks)
        {
            if (rejectSave) return juce::Result::fail("Simulated storage refusal");
            return controller.execute([&](WorkspaceCommands& cmd) { return cmd.replacePageBlocks(id, expected, blocks); });
        };
    };
    REQUIRE(session.bind(first, read(first), write(first)).wasOk(), "Bind first");
    BlockEditorSession::Blocks blocks;
    const char* types[] = {"text", "heading", "list", "checklist", "link"};
    for (const auto* type : types)
        blocks.push_back({juce::Uuid().toString(), type, juce::String::fromUTF8("Строка\nВторая строка"), "https://example.com", false, juce::String::fromUTF8("Имя")});
    session.stage(blocks);
    REQUIRE(session.dirty() && model.findPage(first)->blocks.empty(), "Draft must not mutate live model");
    REQUIRE(session.flush().wasOk() && model.findPage(first)->blocks == blocks, "Save all block types");
    const auto ids = blocks;
    const auto dawJson = model.catalog.toJson();
    blocks[3].checked = true;
    blocks[0].name = "Renamed";
    blocks[1].type = "text";
    blocks[1].text = "Edited";
    session.stage(blocks);
    REQUIRE(session.flush().wasOk(), "Save check/name/type/text");
    REQUIRE(model.catalog.toJson() == dawJson, "Checklist must not change task status/checkpoints/PROG");
    REQUIRE(session.undo().wasOk() && session.blocks() == ids, "Undo complete block edit");
    REQUIRE(session.redo().wasOk() && session.blocks() == blocks, "Redo edit");
    std::swap(blocks[0], blocks[4]);
    session.stage(blocks); REQUIRE(session.flush().wasOk(), "Reorder");
    const auto reordered = blocks;
    blocks.erase(blocks.begin() + 2);
    session.stage(blocks); REQUIRE(session.flush().wasOk(), "Delete");
    REQUIRE(session.undo().wasOk() && session.blocks() == reordered, "Undo deletion preserves IDs/order");
    REQUIRE(session.undo().wasOk() && session.blocks()[0].id == ids[0].id, "Undo reorder");
    REQUIRE(session.redo().wasOk() && session.blocks() == reordered, "Redo reorder");
    // Each document keeps its own history; switching flushes a pending draft.
    blocks = session.blocks(); blocks[0].text = "Pending before switching"; session.stage(blocks);
    REQUIRE(session.bind(second, read(second), write(second)).wasOk(), "Switch must flush");
    REQUIRE(model.findPage(first)->blocks == blocks, "Switch saved draft");
    BlockEditorSession::Blocks secondBlocks {{juce::Uuid().toString(), "text", "Other", {}, false, "Second block"}};
    session.stage(secondBlocks); REQUIRE(session.flush().wasOk(), "Second save");
    REQUIRE(session.bind(first, read(first), write(first)).wasOk(), "Rebind first");
    REQUIRE(session.undo().wasOk() && model.findPage(second)->blocks == secondBlocks, "Undo first must not alter second");
    // Changes outside this document must survive undo.
    model.catalog.songs[0].localNotes.push_back({"task:" + taskId, "New DAW note"});
    REQUIRE(store.save(model).wasOk(), "DAW save");
    REQUIRE(session.undo().wasOk() && model.catalog.songs[0].localNotes[0].text == "New DAW note", "Undo must preserve unrelated DAW edits");
    // Failed persistence leaves live model/history intact and retains the draft.
    const auto beforeFailure = model.toJson();
    const auto blocksBeforeFailure = model.findPage(first)->blocks;
    blocks = session.blocks(); blocks[0].text = "Unsaved"; session.stage(blocks);
    rejectSave = true;
    REQUIRE(session.flush().failed() && model.toJson() == beforeFailure && session.dirty(), "Failed save changed model or discarded draft");
    REQUIRE(session.bind(second, read(second), write(second)).failed(), "Failed save must block switching");
    REQUIRE(session.undo().failed() && model.toJson() == beforeFailure, "Failed undo changed live model");
    rejectSave = false; REQUIRE(session.flush().wasOk(), "Retry pending save");
    REQUIRE(session.undo().wasOk() && model.toJson() != beforeFailure, "Undo retry"); // revision increments, content restored
    REQUIRE(model.findPage(first)->blocks == blocksBeforeFailure, "Failed save must not pollute history");
    // Reject stale writes rather than overwrite newer content.
    blocks = session.blocks(); blocks[0].text = "Local draft"; session.stage(blocks);
    model.findPage(first)->blocks[0].text = "External content";
    REQUIRE(store.save(model).wasOk(), "External content save");
    REQUIRE(session.flush().failed() && model.findPage(first)->blocks[0].text == "External content", "Stale write accepted");
    REQUIRE(session.reload().wasOk() && !session.dirty() && !session.canUndo(), "Explicit reload");
    blocks = session.blocks(); blocks[0].text = "Disk conflict"; session.stage(blocks);
    const auto live = model.toJson();
    REQUIRE(store.getFile().replaceWithText(live + "\n"), "External file write");
    REQUIRE(session.flush().failed() && model.toJson() == live && session.dirty(), "External file overwrite accepted");
    REQUIRE(session.reload().wasOk(), "Reload draft");
    // Invalid types/duplicate IDs never enter the model.
    blocks = model.findPage(first)->blocks;
    auto invalid = blocks; invalid[0].type = "unknown";
    const auto beforeInvalid = model.toJson();
    REQUIRE(commands.replacePageBlocks(first, blocks, invalid).failed() && model.toJson() == beforeInvalid, "Unknown type accepted");
    invalid = blocks; invalid[1].id = invalid[0].id;
    REQUIRE(commands.replacePageBlocks(first, blocks, invalid).failed(), "Duplicate ID accepted");
    REQUIRE(commands.archivePage(first).wasOk(), "Archive");
    REQUIRE(commands.replacePageBlocks(first, blocks, blocks).failed(), "Archived blocks edited");
    REQUIRE(commands.restorePage(first).wasOk(), "Restore");
    REQUIRE(model.findPage(first)->blocks == blocks, "Archive lost blocks");
    WorkspaceStore restartedStore(store.getFile());
    WorkspaceModel restarted;
    REQUIRE(restartedStore.load({}, {}, restarted).wasOk(), "Reload stored workspace after restart");
    REQUIRE(restarted.findPage(first)->blocks == model.findPage(first)->blocks, "Restart lost blocks/IDs/order");
    // Schema 1 documents without the optional name still load.
    WorkspaceModel roundTrip;
    REQUIRE(WorkspaceModel::fromJson(model.toJson(), roundTrip).wasOk(), "Round trip");
    REQUIRE(roundTrip.findPage(first)->blocks == model.findPage(first)->blocks, "Round trip lost name/order/content");
    auto raw = juce::JSON::parse(model.toJson());
    for (auto& page : *raw.getDynamicObject()->getProperty("pages").getArray())
        for (auto& block : *page.getDynamicObject()->getProperty("blocks").getArray())
            block.getDynamicObject()->removeProperty("name");
    WorkspaceModel old;
    REQUIRE(WorkspaceModel::fromJson(juce::JSON::toString(raw), old).wasOk(), "Old schema 1 rejected");
    REQUIRE(old.findPage(first)->blocks[0].name.isEmpty() && old.findPage(first)->blocks[0].id == blocks[0].id, "Legacy name fallback");
    // The editor session supports any block document, not just page ownership.
    BlockEditorSession description;
    BlockEditorSession::Blocks taskDescription;
    REQUIRE(description.bind("task-description", [&]() { return std::optional(taskDescription); },
        [&](const auto& expected, const auto& desired)
        {
            if (taskDescription != expected) return juce::Result::fail("Conflict");
            taskDescription = desired; return juce::Result::ok();
        }).wasOk(), "Bind reusable description");
    description.stage(secondBlocks);
    REQUIRE(description.flush().wasOk() && description.undo().wasOk() && taskDescription.empty(), "Description history");
    std::cout << "Block editor persistence, isolation, conflicts and compatibility passed\n";
    return 0;
}

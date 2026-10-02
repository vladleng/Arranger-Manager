#pragma once
#include "WorkspaceModel.h"
#include <functional>
#include <map>
#include <optional>

namespace arranger
{
// Document callbacks allow the same editor/session to serve page blocks and
// future task descriptions. History contains only the bound document's blocks.
class BlockEditorSession
{
public:
    using Blocks = std::vector<WorkspaceBlock>;
    using Read = std::function<std::optional<Blocks>()>;
    using Write = std::function<juce::Result(const Blocks&, const Blocks&)>;
    juce::Result bind(const juce::String& key, Read readDocument, Write writeDocument)
    {
        if (key == documentId) return refresh();
        const auto saved = flush();
        if (saved.failed()) return saved;
        auto current = readDocument ? readDocument() : std::optional<Blocks>(Blocks{});
        if (!current) return juce::Result::fail("Document is no longer editable.");
        documentId = key;
        read = std::move(readDocument);
        write = std::move(writeDocument);
        auto& state = histories[documentId];
        if (state.initialised && state.last != *current) { state.undo.clear(); state.redo.clear(); }
        baseline = draft = *current;
        state.last = baseline;
        state.initialised = true;
        return juce::Result::ok();
    }
    juce::Result refresh()
    {
        if (!read) return juce::Result::ok();
        const auto current = read();
        if (!current) return juce::Result::fail("Document is no longer editable.");
        if (*current == baseline) return juce::Result::ok();
        if (dirty()) return juce::Result::fail("Document changed while edits were pending.");
        auto& state = histories[documentId];
        state.undo.clear(); state.redo.clear();
        baseline = draft = state.last = *current;
        return juce::Result::ok();
    }
    const Blocks& blocks() const { return draft; }
    bool dirty() const { return draft != baseline; }
    bool canUndo() const { const auto it = histories.find(documentId); return dirty() || (it != histories.end() && !it->second.undo.empty()); }
    bool canRedo() const { const auto it = histories.find(documentId); return !dirty() && it != histories.end() && !it->second.redo.empty(); }
    void stage(Blocks blocks) { draft = std::move(blocks); }
    juce::Result flush()
    {
        if (!dirty()) return juce::Result::ok();
        if (!write) return juce::Result::fail("No writable document.");
        const auto result = write(baseline, draft);
        if (result.failed()) return result;
        auto& state = histories[documentId];
        state.undo.push_back(baseline);
        if (state.undo.size() > 100) state.undo.erase(state.undo.begin());
        state.redo.clear();
        baseline = state.last = draft;
        return juce::Result::ok();
    }
    juce::Result undo()
    {
        const auto result = flush();
        if (result.failed()) return result;
        return travel(false);
    }
    juce::Result redo()
    {
        if (dirty()) return juce::Result::fail("Save pending edits before redo.");
        return travel(true);
    }
    // Explicit reload discards only the editor draft; used after a conflict.
    juce::Result reload()
    {
        if (!read) return juce::Result::ok();
        const auto current = read();
        if (!current) return juce::Result::fail("Document is no longer editable.");
        auto& state = histories[documentId];
        state.undo.clear(); state.redo.clear();
        baseline = draft = state.last = *current;
        return juce::Result::ok();
    }
private:
    struct History { Blocks last; std::vector<Blocks> undo, redo; bool initialised = false; };
    juce::Result travel(bool forwards)
    {
        auto& state = histories[documentId];
        auto& source = forwards ? state.redo : state.undo;
        auto& target = forwards ? state.undo : state.redo;
        if (source.empty()) return juce::Result::ok();
        const auto result = write(baseline, source.back());
        if (result.failed()) return result;
        target.push_back(baseline);
        baseline = draft = state.last = source.back();
        source.pop_back();
        return juce::Result::ok();
    }
    juce::String documentId;
    Read read;
    Write write;
    Blocks baseline, draft;
    std::map<juce::String, History> histories;
};
}

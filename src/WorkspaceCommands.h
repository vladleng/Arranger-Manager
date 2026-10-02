#pragma once
#include "WorkspaceQueries.h"

namespace arranger
{
// Page ownership and mutation rules live here; UI has no storage/file knowledge.
class WorkspaceCommands
{
public:
    explicit WorkspaceCommands(WorkspaceModel& value) : model(value) {}

    juce::Result createPage(const juce::String& parentId, juce::String title, juce::String& createdId)
    {
        title = title.trim();
        if (title.isEmpty()) return juce::Result::fail("Page title cannot be empty.");
        const auto parent = validParent(parentId);
        if (parent.failed()) return parent;
        const auto id = juce::Uuid().toString();
        model.pages.push_back({id, parentId, title, "page", {}, {}});
        createdId = id;
        return juce::Result::ok();
    }

    juce::Result renamePage(const juce::String& id, juce::String title)
    {
        auto* page = editablePage(id);
        if (page == nullptr) return juce::Result::fail("Only active ordinary pages can be renamed.");
        title = title.trim();
        if (title.isEmpty()) return juce::Result::fail("Page title cannot be empty.");
        page->title = title;
        return juce::Result::ok();
    }

    juce::Result movePage(const juce::String& id, const juce::String& parentId)
    {
        auto* page = editablePage(id);
        if (page == nullptr) return juce::Result::fail("Only active ordinary pages can be moved.");
        const auto parent = validParent(parentId);
        if (parent.failed()) return parent;
        if (parentId.isNotEmpty() && WorkspaceQueries::isDescendant(model, parentId, id))
            return juce::Result::fail("A page cannot be nested inside itself or its descendants.");
        page->parentPageId = parentId;
        return juce::Result::ok();
    }

    juce::Result archivePage(const juce::String& id)
    {
        auto* page = editablePage(id);
        if (page == nullptr) return juce::Result::fail("Only active ordinary pages can be archived.");
        // Descendants inherit archive state. Existing child archives are retained
        // when a parent is later restored, and no blocks/IDs are removed.
        page->archivedAt = juce::Time::getCurrentTime().toISO8601(true);
        return juce::Result::ok();
    }

    juce::Result restorePage(const juce::String& id)
    {
        auto* page = model.findPage(id);
        if (page == nullptr || page->kind != "page" || page->archivedAt.isEmpty())
            return juce::Result::fail("Archived page was not found.");
        if (WorkspaceQueries::isArchived(model, page->parentPageId))
            return juce::Result::fail("Restore the archived parent first.");
        page->archivedAt.clear();
        return juce::Result::ok();
    }

private:
    WorkspacePage* editablePage(const juce::String& id)
    {
        auto* page = model.findPage(id);
        return page != nullptr && page->kind == "page" && !WorkspaceQueries::isArchived(model, id) ? page : nullptr;
    }
    juce::Result validParent(const juce::String& id) const
    {
        if (id.isEmpty()) return juce::Result::ok();
        const auto* page = model.findPage(id);
        if (page == nullptr || page->kind != "page" || WorkspaceQueries::isArchived(model, id))
            return juce::Result::fail("Choose an active ordinary page as parent.");
        return juce::Result::ok();
    }
    WorkspaceModel& model;
};
using WorkspaceAction = std::function<juce::Result(WorkspaceCommands&)>;
}

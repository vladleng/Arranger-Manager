#pragma once
#include <JuceHeader.h>
#include "SongSnapshot.h"
#include "SongCatalog.h"
#include "ArrangementTag.h"
#include <functional>
#include <map>
#include <set>
#include <vector>

class HubEditor final : public juce::Component, public juce::DragAndDropContainer,
    private juce::Timer, private juce::ListBoxModel
{
public:
    HubEditor(std::function<juce::String()> getProjectPath,
        std::function<void(juce::String)> setProjectPath,
        arranger::SongCatalog* catalog = nullptr, std::function<void()> saveCatalog = {});
    ~HubEditor() override;
    void paint(juce::Graphics&) override;
    void resized() override;
private:
    enum class RowKind { folder, project, task, checkpoint, track, clip };
    enum class EditKind { none, note, name };
    class TaskList final : public juce::ListBox, public juce::DragAndDropTarget
    {
    public:
        explicit TaskList(HubEditor& owner) : juce::ListBox("Project events", &owner), owner(owner) {}
        bool isInterestedInDragSource(const SourceDetails& details) override;
        void itemDragMove(const SourceDetails& details) override;
        void itemDragExit(const SourceDetails&) override;
        void itemDropped(const SourceDetails& details) override;
        void paintOverChildren(juce::Graphics& g) override;
    private:
        HubEditor& owner;
        int dropRow = -1;
        bool after = false;
    };
    struct SectionLabel { juce::String name; juce::Colour colour; };
    struct Row
    {
        RowKind kind;
        juce::String title, detail, position, summary;
        arranger::Tag tag;
        std::string key;
        int depth = 0;
        bool expandable = false, expanded = false;
        juce::String notes, noteKey;
        std::vector<SectionLabel> sections;
        juce::Colour trackColour;
        bool midi = false;
        juce::String mediaType;
        int done = 0, total = 0;
        juce::String songPath, folderId, taskId, checkpointId;
        arranger::Status manualStatus = arranger::Status::unmarked;
        arranger::SongType songType = arranger::SongType::unspecified;
    };
    struct CachedSong
    {
        arranger::SongSnapshot snapshot;
        juce::int64 modified = -1, size = -1;
    };
    void timerCallback() override;
    void refreshSnapshot();
    void refreshCatalog();
    void updateHeader(const juce::String& selectedPath);
    void rebuildRows();
    void appendSongRows(const arranger::SongSnapshot&, int depth, const std::string& key,
        const juce::String& songPath, bool catalogProject);
    void promptNewFolder();
    void showSongMenu(const juce::String& songPath);
    void showFolderMenu(const juce::String& folderId);
    void showStatusMenu(const juce::String& songPath);
    void showSongTypeMenu(const Row& row);
    void showTaskMenu(const Row& row);
    void showLocalStatusMenu(const Row& row);
    void promptLocalNote(const Row& row);
    void beginInlineEdit(int index, EditKind kind);
    void finishInlineEdit(bool save);
    void positionInlineEditor();
    bool dropTarget(const juce::String& sourceKey, int x, int y, int& targetIndex, bool& after) const;
    void reorderDrop(const juce::String& sourceKey, int index, bool after);
    void showTrackOrClipMenu(const Row& row);
    void promptLocalName(const juce::String& songPath, const juce::String& taskId,
        const juce::String& checkpointId = {}, bool rename = false);
    void catalogChanged();
    int getNumRows() override { return static_cast<int>(rows.size()); }
    juce::String getTooltipForRow(int row) override;
    void paintListBoxItem(int, juce::Graphics&, int, int, bool) override;
    void listBoxItemClicked(int, const juce::MouseEvent&) override;
    void listBoxItemDoubleClicked(int, const juce::MouseEvent&) override;
    juce::var getDragSourceDescription(const juce::SparseSet<int>&) override;
    bool mayDragToExternalWindows() const override { return false; }
    void listWasScrolled() override;

    std::function<juce::String()> getProjectPath;
    std::function<void(juce::String)> setProjectPath;
    arranger::SongCatalog* catalog;
    std::function<void()> saveCatalog;
    juce::Label heading, path, summary, info, notes, help;
    juce::TextButton choose { "Open .song" }, newFolder { "New folder" }, refresh { "Refresh" };
    TaskList list { *this };
    juce::TextEditor inlineEditor { "Inline edit" };
    EditKind editKind = EditKind::none;
    Row editRow { RowKind::folder };
    int editIndex = -1;
    juce::TooltipWindow tooltipWindow { this, 650 };
    std::unique_ptr<juce::FileChooser> chooser;
    arranger::SongSnapshot snapshot;
    std::vector<Row> rows;
    std::set<std::string> collapsed;
    std::set<std::string> expandedSongs;
    std::set<std::string> expandedChildren;
    std::map<std::string, CachedSong> songCache;
    juce::String selectedFolderId;
    juce::String lastPath;
    juce::int64 lastModified = -1, lastSize = -1;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(HubEditor)
};

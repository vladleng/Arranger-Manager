#pragma once
#include <JuceHeader.h>
#include "SongSnapshot.h"
#include "SongCatalog.h"
#include "ArrangementTag.h"
#include <functional>
#include <map>
#include <set>
#include <vector>

class HubEditor final : public juce::Component, private juce::Timer, private juce::ListBoxModel
{
public:
    HubEditor(std::function<juce::String()> getProjectPath,
        std::function<void(juce::String)> setProjectPath,
        arranger::SongCatalog* catalog = nullptr, std::function<void()> saveCatalog = {});
    ~HubEditor() override;
    void paint(juce::Graphics&) override;
    void resized() override;
private:
    enum class RowKind { folder, project, track, clip };
    struct SectionLabel { juce::String name; juce::Colour colour; };
    struct Row
    {
        RowKind kind;
        juce::String title, detail, position, summary;
        arranger::Tag tag;
        std::string key;
        int depth = 0;
        bool expandable = false, expanded = false;
        juce::String notes;
        std::vector<SectionLabel> sections;
        juce::Colour trackColour;
        bool midi = false;
        juce::String mediaType;
        int done = 0, total = 0;
        juce::String songPath, folderId;
        arranger::Status manualStatus = arranger::Status::unmarked;
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
    void catalogChanged();
    int getNumRows() override { return static_cast<int>(rows.size()); }
    void paintListBoxItem(int, juce::Graphics&, int, int, bool) override;
    void listBoxItemClicked(int, const juce::MouseEvent&) override;

    std::function<juce::String()> getProjectPath;
    std::function<void(juce::String)> setProjectPath;
    arranger::SongCatalog* catalog;
    std::function<void()> saveCatalog;
    juce::Label heading, path, summary, info, notes, help;
    juce::TextButton choose { "Open .song" }, newFolder { "New folder" }, refresh { "Refresh" };
    juce::ListBox list { "Project events", this };
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

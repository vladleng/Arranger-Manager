#pragma once
#include <JuceHeader.h>
#include "SongSnapshot.h"
#include "ArrangementTag.h"
#include <functional>
#include <set>
#include <vector>

class HubEditor final : public juce::Component, private juce::Timer, private juce::ListBoxModel
{
public:
    HubEditor(std::function<juce::String()> getProjectPath,
        std::function<void(juce::String)> setProjectPath);
    ~HubEditor() override;
    void paint(juce::Graphics&) override;
    void resized() override;
private:
    enum class RowKind { project, group, track, clip, marker };
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
    };
    void timerCallback() override;
    void refreshSnapshot();
    void rebuildRows();
    int getNumRows() override { return static_cast<int>(rows.size()); }
    void paintListBoxItem(int, juce::Graphics&, int, int, bool) override;
    void listBoxItemClicked(int, const juce::MouseEvent&) override;

    std::function<juce::String()> getProjectPath;
    std::function<void(juce::String)> setProjectPath;
    juce::Label heading, path, summary, info, notes, help;
    juce::TextButton choose { "Open .song" }, refresh { "Refresh" };
    juce::ListBox list { "Project events", this };
    std::unique_ptr<juce::FileChooser> chooser;
    arranger::SongSnapshot snapshot;
    std::vector<Row> rows;
    std::set<std::string> collapsed;
    juce::String lastPath;
    juce::int64 lastModified = -1, lastSize = -1;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(HubEditor)
};

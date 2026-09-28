#pragma once
#include <JuceHeader.h>
#include "HubProcessor.h"
#include "SongSnapshot.h"
#include "ArrangementTag.h"
#include <set>
#include <vector>

class HubEditor final : public juce::AudioProcessorEditor, private juce::Timer, private juce::ListBoxModel
{
public:
    explicit HubEditor(HubProcessor&);
    ~HubEditor() override;
    void paint(juce::Graphics&) override;
    void resized() override;
private:
    enum class RowKind { project, group, track, clip, section, marker };
    struct Row
    {
        RowKind kind;
        juce::String title, detail, position, summary;
        arranger::Tag tag;
        std::string key;
        int depth = 0;
        bool expandable = false, expanded = false;
    };
    void timerCallback() override;
    void refreshSnapshot();
    void rebuildRows();
    int getNumRows() override { return static_cast<int>(rows.size()); }
    void paintListBoxItem(int, juce::Graphics&, int, int, bool) override;
    void listBoxItemClicked(int, const juce::MouseEvent&) override;

    HubProcessor& processor;
    juce::Label heading, path, summary, info, notes, help;
    juce::TextButton choose { "Open .song" }, refresh { "Refresh" };
    juce::ListBox list { "Project events", this };
    std::unique_ptr<juce::FileChooser> chooser;
    arranger::SongSnapshot snapshot;
    std::vector<Row> rows;
    std::set<std::string> collapsed;
    std::set<std::string> expandedSections;
    juce::String lastPath;
    juce::int64 lastModified = -1, lastSize = -1;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(HubEditor)
};

#pragma once
#include <JuceHeader.h>
#include "HubProcessor.h"
#include "SongSnapshot.h"
#include "ArrangementTag.h"
#include <vector>

class HubEditor final : public juce::AudioProcessorEditor, private juce::Timer, private juce::ListBoxModel
{
public:
    explicit HubEditor(HubProcessor&);
    ~HubEditor() override;
    void paint(juce::Graphics&) override;
    void resized() override;
private:
    struct Row { arranger::SongEvent event; arranger::Tag tag; };
    void timerCallback() override;
    void refreshSnapshot();
    int getNumRows() override { return static_cast<int>(rows.size()); }
    void paintListBoxItem(int, juce::Graphics&, int, int, bool) override;

    HubProcessor& processor;
    juce::Label heading, path, summary, info, notes, help;
    juce::TextButton choose { "Open .song" }, refresh { "Refresh" };
    juce::ListBox list { "Project events", this };
    std::unique_ptr<juce::FileChooser> chooser;
    std::vector<Row> rows;
    juce::String lastPath;
    juce::int64 lastModified = -1, lastSize = -1;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(HubEditor)
};

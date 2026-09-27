#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "ArrangementTag.h"
#include <vector>

class InspectorEditor final : public juce::AudioProcessorEditor, private juce::Timer, private juce::ListBoxModel
{
public:
    explicit InspectorEditor(InspectorProcessor&);
    ~InspectorEditor() override;
    void paint(juce::Graphics&) override;
    void resized() override;
private:
    void timerCallback() override;
    int getNumRows() override;
    void paintListBoxItem(int row, juce::Graphics&, int width, int height, bool selected) override;
    juce::String report() const;
    struct Row
    {
        juce::String track, name;
        arranger::Tag tag;
        double start = 0.0, duration = 0.0;
    };
    std::vector<Row> rows;
    InspectorProcessor& processor;
    juce::Label title;
    juce::Label help;
    juce::ListBox list { "Arrangement regions", this };
    juce::TextEditor output;
    juce::TextButton copy { "Copy report" };
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(InspectorEditor)
};

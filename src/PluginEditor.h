#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "ArrangementTag.h"
#include <memory>
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
    void importContext(const juce::File&);
    juce::String sectionsFor(double start, double duration) const;
    void updateContextSummary();
    struct ContextEvent
    {
        juce::String name;
        double start = 0.0, end = 0.0;
        int colour = 0;
    };
    struct Row
    {
        juce::String track, name;
        arranger::Tag tag;
        double start = 0.0, duration = 0.0;
    };
    std::vector<Row> rows;
    std::vector<ContextEvent> sections, markers;
    std::unique_ptr<juce::FileChooser> contextChooser;
    InspectorProcessor& processor;
    juce::Label title;
    juce::Label help;
    juce::ListBox list { "Arrangement regions", this };
    juce::TextEditor output;
    juce::TextButton copy { "Copy report" };
    juce::TextButton importButton { "Import JSON" };
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(InspectorEditor)
};

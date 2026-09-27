#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"

class InspectorEditor final : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit InspectorEditor(InspectorProcessor&);
    ~InspectorEditor() override;
    void paint(juce::Graphics&) override;
    void resized() override;
private:
    void timerCallback() override;
    juce::String report() const;
    InspectorProcessor& processor;
    juce::Label title;
    juce::TextEditor output;
    juce::TextButton copy { "Copy report" };
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(InspectorEditor)
};

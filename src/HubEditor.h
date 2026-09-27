#pragma once
#include <JuceHeader.h>
#include "HubProcessor.h"
#include "SharedArrangementMap.h"
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
    struct Row
    {
        juce::String track, name;
        arranger::Tag tag;
        double start = 0.0, duration = 0.0;
    };
    void timerCallback() override;
    int getNumRows() override { return static_cast<int>(rows.size()); }
    void paintListBoxItem(int, juce::Graphics&, int, int, bool) override;

    juce::Label heading, summary, help;
    juce::ListBox list { "Arrangement map", this };
    std::vector<Row> rows;
    std::uint64_t lastRevision = ~std::uint64_t{};
};

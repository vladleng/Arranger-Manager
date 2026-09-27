#pragma once
#include <JuceHeader.h>
#include <atomic>

class InspectorProcessor final : public juce::AudioProcessor
#if JucePlugin_Enable_ARA
    , public juce::AudioProcessorARAExtension
#endif
{
public:
    InspectorProcessor();
    void prepareToPlay(double, int) override {}
    void releaseResources() override {}
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}
    void getStateInformation(juce::MemoryBlock&) override;
    void setStateInformation(const void*, int) override {}
    bool isBound() const { return bound.load(std::memory_order_relaxed); }
protected:
#if JucePlugin_Enable_ARA
    void didBindToARA() noexcept override;
#endif
private:
    std::atomic<bool> bound { false };
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(InspectorProcessor)
};

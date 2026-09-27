#include "HubProcessor.h"
#include "HubEditor.h"

HubProcessor::HubProcessor()
    : juce::AudioProcessor(BusesProperties().withInput("Input", juce::AudioChannelSet::stereo(), true)
                                       .withOutput("Output", juce::AudioChannelSet::stereo(), true)) {}

bool HubProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    const auto input = layouts.getMainInputChannelSet();
    return input == layouts.getMainOutputChannelSet()
        && (input == juce::AudioChannelSet::mono() || input == juce::AudioChannelSet::stereo());
}

juce::AudioProcessorEditor* HubProcessor::createEditor() { return new HubEditor(*this); }
void HubProcessor::getStateInformation(juce::MemoryBlock& data)
{
    static constexpr char marker[] = "ArrangerManagerHubV1";
    data.replaceAll(marker, sizeof(marker));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new HubProcessor(); }

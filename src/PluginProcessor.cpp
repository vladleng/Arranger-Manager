#include "PluginProcessor.h"
#include "PluginEditor.h"
#if JucePlugin_Enable_ARA
#include "InspectorDocumentController.h"
#endif

InspectorProcessor::InspectorProcessor()
    : juce::AudioProcessor(BusesProperties().withInput("Input", juce::AudioChannelSet::stereo(), true)
                                       .withOutput("Output", juce::AudioChannelSet::stereo(), true)) {}

bool InspectorProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    const auto input = layouts.getMainInputChannelSet();
    return input == layouts.getMainOutputChannelSet()
        && (input == juce::AudioChannelSet::mono() || input == juce::AudioChannelSet::stereo());
}

void InspectorProcessor::processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) {}
juce::AudioProcessorEditor* InspectorProcessor::createEditor() { return new InspectorEditor(*this); }
void InspectorProcessor::getStateInformation(juce::MemoryBlock& data)
{
    static constexpr char marker[] = "ArrangerManagerInspectorV1";
    data.replaceAll(marker, sizeof(marker));
}

#if JucePlugin_Enable_ARA
void InspectorProcessor::didBindToARA() noexcept
{
    juce::AudioProcessorARAExtension::didBindToARA();
    bound.store(true, std::memory_order_relaxed);
}
#endif

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new InspectorProcessor(); }
#if JucePlugin_Enable_ARA
const ARA::ARAFactory* JUCE_CALLTYPE createARAFactory()
{
    return juce::ARADocumentControllerSpecialisation::createARAFactory<InspectorDocumentController>();
}
#endif

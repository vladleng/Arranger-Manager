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

namespace
{
class HubPluginEditor final : public juce::AudioProcessorEditor
{
public:
    explicit HubPluginEditor(HubProcessor& processor)
        : juce::AudioProcessorEditor(&processor),
          view([&processor] { return processor.getProjectPath(); },
              [&processor](juce::String path) { processor.setProjectPath(std::move(path)); })
    {
        addAndMakeVisible(view);
        setResizable(true, true);
        setResizeLimits(460, 320, 1600, 1200);
        setSize(view.getWidth(), view.getHeight());
    }

    void resized() override { view.setBounds(getLocalBounds()); }

private:
    HubEditor view;
};
}

juce::AudioProcessorEditor* HubProcessor::createEditor() { return new HubPluginEditor(*this); }
void HubProcessor::getStateInformation(juce::MemoryBlock& data)
{
    juce::XmlElement state("ArrangerManagerHubState");
    state.setAttribute("version", 1);
    state.setAttribute("projectPath", getProjectPath());
    const auto xml = state.toString();
    data.replaceAll(xml.toRawUTF8(), static_cast<size_t>(xml.getNumBytesAsUTF8()));
}

void HubProcessor::setStateInformation(const void* bytes, int size)
{
    if (bytes == nullptr || size <= 0) return;
    auto state = juce::XmlDocument::parse(juce::String::fromUTF8(static_cast<const char*>(bytes), size));
    if (state != nullptr && state->hasTagName("ArrangerManagerHubState"))
        setProjectPath(state->getStringAttribute("projectPath"));
}

juce::String HubProcessor::getProjectPath() const
{
    const juce::ScopedLock lock(stateLock);
    return projectPath;
}

void HubProcessor::setProjectPath(juce::String path)
{
    {
        const juce::ScopedLock lock(stateLock);
        projectPath = std::move(path);
    }
    updateHostDisplay();
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new HubProcessor(); }

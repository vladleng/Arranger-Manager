#include "PluginEditor.h"
#include "InspectorState.h"

InspectorEditor::InspectorEditor(InspectorProcessor& p) : juce::AudioProcessorEditor(&p), processor(p)
{
    title.setText("Arranger Manager — ARA Inspector 0.0a", juce::dontSendNotification);
    title.setFont(juce::FontOptions(21.0f, juce::Font::bold));
    addAndMakeVisible(title);
    output.setMultiLine(true);
    output.setReadOnly(true);
    output.setScrollbarsShown(true);
    output.setFont(juce::FontOptions(15.0f));
    addAndMakeVisible(output);
    copy.onClick = [this] { juce::SystemClipboard::copyTextToClipboard(report()); };
    addAndMakeVisible(copy);
    setSize(920, 650);
    timerCallback();
    startTimerHz(4);
}

InspectorEditor::~InspectorEditor() { stopTimer(); }
void InspectorEditor::paint(juce::Graphics& g) { g.fillAll(juce::Colour(0xff1d232b)); }
void InspectorEditor::resized()
{
    auto area = getLocalBounds().reduced(16);
    auto header = area.removeFromTop(44);
    copy.setBounds(header.removeFromRight(145).reduced(4));
    title.setBounds(header);
    output.setBounds(area);
}

void InspectorEditor::timerCallback()
{
    const auto text = report();
    if (text != output.getText()) output.setText(text, false);
}

juce::String InspectorEditor::report() const
{
    juce::String text;
    const auto documents = InspectorState::instance().read();
    text << "ARA bound: " << (processor.isBound() ? "YES" : "NO")
         << "  |  visible document controllers: " << documents.size() << "\n";
    text << "Time unit: seconds. Names/colors marked <not provided> are absent in ARA.\n";
    if (documents.empty()) text << "\nNo ARA document. Add this plug-in as an ARA/Event FX on an audio event.\n";
    for (size_t di = 0; di < documents.size(); ++di)
    {
        const auto& d = documents[di];
        text << "\nDOCUMENT " << (di + 1) << ": " << d.documentName
             << " | contexts " << d.musicalContexts << " | sequences " << d.sequenceCount
             << " | regions " << d.regionCount << " | revision " << juce::String(static_cast<juce::int64>(d.revision))
             << " | last: " << d.lastChange << "\n\n";
        for (const auto& row : d.regions)
        {
            text << "Sequence #" << row.sequenceIndex << "  " << row.sequenceName
                 << "  [color " << row.sequenceColor << "]\n"
                 << "  Region #" << row.regionIndex << "  explicit name: " << row.regionName
                 << "  | effective name: " << row.effectiveName << "\n"
                 << "  Start " << juce::String(row.startSeconds, 4)
                 << " s  |  Duration " << juce::String(row.durationSeconds, 4)
                 << " s  |  Region color " << row.regionColor << "\n\n";
        }
        for (const auto& empty : d.emptySequences) text << "Empty sequence: " << empty << "\n";
    }
    return text;
}

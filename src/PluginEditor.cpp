#include "PluginEditor.h"
#include "InspectorState.h"

namespace
{
juce::Colour statusColor(arranger::Status status)
{
    switch (status)
    {
        case arranger::Status::todo: return juce::Colour(0xff818a96);
        case arranger::Status::wip: return juce::Colour(0xff3b82f6);
        case arranger::Status::draft: return juce::Colour(0xff35b8d6);
        case arranger::Status::review: return juce::Colour(0xffe9b949);
        case arranger::Status::done: return juce::Colour(0xff34a878);
        case arranger::Status::blocked: return juce::Colour(0xffdd5b61);
        default: return juce::Colour(0xff56606a);
    }
}

juce::Colour priorityColor(arranger::Priority priority)
{
    switch (priority)
    {
        case arranger::Priority::p0: return juce::Colour(0xffdd5b61);
        case arranger::Priority::p1: return juce::Colour(0xffed9844);
        case arranger::Priority::p2: return juce::Colour(0xff679fe8);
        case arranger::Priority::p3: return juce::Colour(0xff929aaa);
        default: return juce::Colour(0xff56606a);
    }
}

void chip(juce::Graphics& g, juce::Rectangle<int> area, juce::Colour color, const juce::String& label)
{
    g.setColour(color);
    g.fillRoundedRectangle(area.toFloat(), 5.0f);
    g.setColour(juce::Colours::white);
    g.setFont(juce::FontOptions(13.0f, juce::Font::bold));
    g.drawText(label, area, juce::Justification::centred);
}
}

InspectorEditor::InspectorEditor(InspectorProcessor& p) : juce::AudioProcessorEditor(&p), processor(p)
{
    title.setText("Arranger Manager - Map Preview 0.0b", juce::dontSendNotification);
    title.setFont(juce::FontOptions(21.0f, juce::Font::bold));
    addAndMakeVisible(title);
    help.setText("Event: STATUS | P1 | note    /    event color = status, priority badge = separate color", juce::dontSendNotification);
    help.setColour(juce::Label::textColourId, juce::Colour(0xffb9c2ca));
    help.setFont(juce::FontOptions(13.0f));
    addAndMakeVisible(help);
    list.setRowHeight(38);
    list.setColour(juce::ListBox::backgroundColourId, juce::Colour(0xff222b33));
    addAndMakeVisible(list);
    output.setMultiLine(true);
    output.setReadOnly(true);
    output.setScrollbarsShown(true);
    output.setFont(juce::FontOptions(15.0f));
    addAndMakeVisible(output);
    copy.onClick = [this] { juce::SystemClipboard::copyTextToClipboard(report()); };
    addAndMakeVisible(copy);
    setSize(960, 720);
    timerCallback();
    startTimerHz(4);
}

InspectorEditor::~InspectorEditor() { stopTimer(); }
void InspectorEditor::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff1d232b));
    const arranger::Status statuses[] = { arranger::Status::todo, arranger::Status::wip, arranger::Status::draft,
        arranger::Status::review, arranger::Status::done, arranger::Status::blocked };
    int x = 18;
    for (const auto status : statuses)
    {
        chip(g, {x, 62, 92, 24}, statusColor(status), arranger::label(status));
        x += 100;
    }
    const arranger::Priority priorities[] = { arranger::Priority::p0, arranger::Priority::p1,
        arranger::Priority::p2, arranger::Priority::p3 };
    x = 18;
    for (const auto priority : priorities)
    {
        chip(g, {x, 93, 68, 24}, priorityColor(priority), arranger::label(priority));
        x += 76;
    }
}
void InspectorEditor::resized()
{
    auto area = getLocalBounds().reduced(16);
    auto header = area.removeFromTop(44);
    copy.setBounds(header.removeFromRight(145).reduced(4));
    title.setBounds(header);
    area.removeFromTop(60);
    help.setBounds(area.removeFromTop(30));
    list.setBounds(area.removeFromTop((area.getHeight() * 52) / 100));
    area.removeFromTop(10);
    output.setBounds(area);
}

void InspectorEditor::timerCallback()
{
    std::vector<Row> next;
    for (const auto& document : InspectorState::instance().read())
        for (const auto& region : document.regions)
        {
            Row row;
            row.track = region.sequenceName;
            row.name = region.regionName;
            row.tag = arranger::parse(region.regionName.toStdString());
            row.start = region.startSeconds;
            row.duration = region.durationSeconds;
            next.push_back(std::move(row));
        }
    rows = std::move(next);
    list.updateContent();
    list.repaint();
    const auto text = report();
    if (text != output.getText()) output.setText(text, false);
}

int InspectorEditor::getNumRows() { return static_cast<int>(rows.size()); }

void InspectorEditor::paintListBoxItem(int rowIndex, juce::Graphics& g, int width, int height, bool selected)
{
    if (rowIndex < 0 || rowIndex >= static_cast<int>(rows.size())) return;
    const auto& row = rows[static_cast<size_t>(rowIndex)];
    g.fillAll(selected ? juce::Colour(0xff354554) : (rowIndex % 2 ? juce::Colour(0xff26313b) : juce::Colour(0xff222b33)));
    chip(g, {9, 7, 92, height - 14}, statusColor(row.tag.status), arranger::label(row.tag.status));
    chip(g, {109, 7, 46, height - 14}, priorityColor(row.tag.priority), arranger::label(row.tag.priority));
    g.setColour(juce::Colour(0xffdce4eb));
    g.setFont(juce::FontOptions(14.0f));
    g.drawFittedText(row.track, {165, 0, 150, height}, juce::Justification::centredLeft, 1);
    const auto note = row.tag.valid && ! row.tag.note.empty()
        ? juce::String::fromUTF8(row.tag.note.c_str()) : row.name;
    g.drawFittedText(note, {320, 0, width - 480, height}, juce::Justification::centredLeft, 1);
    g.setColour(juce::Colour(0xffaebdca));
    g.drawText(juce::String(row.start, 1) + " - " + juce::String(row.start + row.duration, 1) + " s",
               {width - 155, 0, 145, height}, juce::Justification::centredRight);
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

#include "PluginEditor.h"
#include "InspectorState.h"
#include <algorithm>

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
    title.setText("Arranger Manager - Map Preview 0.0d", juce::dontSendNotification);
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
    setResizable(true, true);
    setResizeLimits(320, 240, 1800, 1400);
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
    const bool compact = getWidth() < 650;
    int index = 0;
    for (const auto status : statuses)
    {
        const int column = compact ? index % 3 : index;
        const int row = compact ? index / 3 : 0;
        chip(g, {18 + column * 100, 62 + row * 30, 92, 24}, statusColor(status), arranger::label(status));
        ++index;
    }
    const arranger::Priority priorities[] = { arranger::Priority::p0, arranger::Priority::p1,
        arranger::Priority::p2, arranger::Priority::p3 };
    int x = 18;
    for (const auto priority : priorities)
    {
        chip(g, {x, compact ? 123 : 93, 68, 24}, priorityColor(priority), arranger::label(priority));
        x += 76;
    }
}
void InspectorEditor::resized()
{
    // The host may briefly report zero or tiny dimensions while reparenting.
    if (getWidth() < 320 || getHeight() < 180)
    {
        title.setBounds(0, 0, 0, 0);
        copy.setBounds(0, 0, 0, 0);
        help.setBounds(0, 0, 0, 0);
        list.setBounds(0, 0, 0, 0);
        output.setBounds(0, 0, 0, 0);
        return;
    }
    auto area = getLocalBounds().reduced(12);
    auto header = area.removeFromTop(40);
    copy.setBounds(header.removeFromRight(120).reduced(3));
    title.setBounds(header);
    area.removeFromTop(getWidth() < 650 ? 96 : 66);
    const bool showHelp = getHeight() >= 350;
    help.setVisible(showHelp);
    help.setBounds(showHelp ? area.removeFromTop(26) : juce::Rectangle<int>{});
    const bool showReport = getHeight() >= 500;
    output.setVisible(showReport);
    if (showReport)
    {
        list.setBounds(area.removeFromTop(std::max(0, (area.getHeight() * 60) / 100)));
        area.removeFromTop(8);
        output.setBounds(area);
    }
    else
    {
        list.setBounds(area);
        output.setBounds({});
    }
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
    const auto changed = next.size() != rows.size()
        || ! std::equal(next.begin(), next.end(), rows.begin(), [](const Row& a, const Row& b)
        {
            return a.track == b.track && a.name == b.name && a.start == b.start && a.duration == b.duration;
        });
    if (changed)
    {
        rows = std::move(next);
        list.updateContent();
        list.repaint();
    }
    const auto text = report();
    if (text != output.getText()) output.setText(text, false);
}

int InspectorEditor::getNumRows() { return static_cast<int>(rows.size()); }

void InspectorEditor::paintListBoxItem(int rowIndex, juce::Graphics& g, int width, int height, bool selected)
{
    if (rowIndex < 0 || rowIndex >= static_cast<int>(rows.size()) || width <= 0 || height <= 0) return;
    const auto& row = rows[static_cast<size_t>(rowIndex)];
    g.fillAll(selected ? juce::Colour(0xff354554) : (rowIndex % 2 ? juce::Colour(0xff26313b) : juce::Colour(0xff222b33)));
    const auto chipHeight = std::max(1, height - 14);
    chip(g, {9, 7, 92, chipHeight}, statusColor(row.tag.status), arranger::label(row.tag.status));
    if (width < 160) return;
    chip(g, {109, 7, 46, chipHeight}, priorityColor(row.tag.priority), arranger::label(row.tag.priority));
    if (width < 340) return;
    g.setColour(juce::Colour(0xffdce4eb));
    g.setFont(juce::FontOptions(14.0f));
    g.drawFittedText(row.track, juce::Rectangle<int> {165, 0, std::min(150, width - 175), height}, juce::Justification::centredLeft, 1);
    const auto note = row.tag.valid && ! row.tag.note.empty()
        ? juce::String::fromUTF8(row.tag.note.c_str()) : row.name;
    if (width > 490)
        g.drawFittedText(note, juce::Rectangle<int> {320, 0, width - 480, height}, juce::Justification::centredLeft, 1);
    g.setColour(juce::Colour(0xffaebdca));
    if (width > 490)
        g.drawText(juce::String(row.start, 1) + " - " + juce::String(row.start + row.duration, 1) + " s",
                   juce::Rectangle<int> {width - 155, 0, 145, height}, juce::Justification::centredRight);
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

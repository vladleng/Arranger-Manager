#include "HubEditor.h"
#include <algorithm>

namespace
{
juce::Colour statusColor(arranger::Status s)
{
    switch (s)
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

juce::Colour priorityColor(arranger::Priority p)
{
    switch (p)
    {
        case arranger::Priority::p0: return juce::Colour(0xffdd5b61);
        case arranger::Priority::p1: return juce::Colour(0xffed9844);
        case arranger::Priority::p2: return juce::Colour(0xff679fe8);
        case arranger::Priority::p3: return juce::Colour(0xff929aaa);
        default: return juce::Colour(0xff56606a);
    }
}

void badge(juce::Graphics& g, juce::Rectangle<int> bounds, juce::Colour color, const char* text)
{
    if (bounds.getWidth() <= 0 || bounds.getHeight() <= 0) return;
    g.setColour(color);
    g.fillRoundedRectangle(bounds.toFloat(), 5.0f);
    g.setColour(juce::Colours::white);
    g.setFont(juce::FontOptions(13.0f, juce::Font::bold));
    g.drawText(text, bounds, juce::Justification::centred);
}
}

HubEditor::HubEditor(HubProcessor& processor) : juce::AudioProcessorEditor(&processor)
{
    heading.setText("Arranger Manager Hub - Map Preview 0.0c", juce::dontSendNotification);
    heading.setFont(juce::FontOptions(21.0f, juce::Font::bold));
    addAndMakeVisible(heading);
    summary.setFont(juce::FontOptions(14.0f));
    addAndMakeVisible(summary);
    help.setText("ARA Event FX on marked clips  |  Hub as a regular track insert", juce::dontSendNotification);
    help.setColour(juce::Label::textColourId, juce::Colour(0xffb9c2ca));
    addAndMakeVisible(help);
    list.setRowHeight(38);
    list.setColour(juce::ListBox::backgroundColourId, juce::Colour(0xff222b33));
    addAndMakeVisible(list);
    setResizable(true, true);
    setResizeLimits(350, 300, 1600, 1200);
    setSize(820, 520);
    timerCallback();
    startTimerHz(4);
}

HubEditor::~HubEditor() { stopTimer(); }

void HubEditor::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff1d232b));
    const arranger::Status statuses[] = { arranger::Status::todo, arranger::Status::wip, arranger::Status::draft,
        arranger::Status::review, arranger::Status::done, arranger::Status::blocked };
    const bool compact = getWidth() < 670;
    int index = 0;
    for (const auto s : statuses)
    {
        const int column = compact ? index % 3 : index;
        const int row = compact ? index / 3 : 0;
        badge(g, {18 + column * 100, 66 + row * 30, 92, 24}, statusColor(s), arranger::label(s));
        ++index;
    }
    const arranger::Priority priorities[] = { arranger::Priority::p0, arranger::Priority::p1,
        arranger::Priority::p2, arranger::Priority::p3 };
    int x = 18;
    for (const auto p : priorities)
    {
        badge(g, {x, compact ? 126 : 97, 68, 24}, priorityColor(p), arranger::label(p));
        x += 76;
    }
}

void HubEditor::resized()
{
    if (getWidth() < 350 || getHeight() < 220)
    {
        heading.setBounds(0, 0, 0, 0);
        summary.setBounds(0, 0, 0, 0);
        help.setBounds(0, 0, 0, 0);
        list.setBounds(0, 0, 0, 0);
        return;
    }
    auto area = getLocalBounds().reduced(16);
    heading.setBounds(area.removeFromTop(44));
    area.removeFromTop(getWidth() < 670 ? 100 : 70);
    summary.setBounds(area.removeFromTop(28));
    help.setBounds(area.removeFromTop(25));
    area.removeFromTop(8);
    list.setBounds(area);
}

void HubEditor::timerCallback()
{
    const auto map = arranger::SharedArrangementMap::instance().read();
    if (map.revision == lastRevision) return;
    lastRevision = map.revision;
    rows.clear();
    for (std::uint32_t i = 0; i < std::min(map.count, static_cast<std::uint32_t>(arranger::maxSharedRegions)); ++i)
    {
        const auto& region = map.regions[i];
        Row row;
        row.track = juce::String::fromUTF8(region.track, sizeof(region.track));
        row.name = juce::String::fromUTF8(region.name, sizeof(region.name));
        row.tag = arranger::parse(row.name.toStdString());
        row.start = region.startSeconds;
        row.duration = region.durationSeconds;
        rows.push_back(std::move(row));
    }
    summary.setText(map.documents == 0 ? "Waiting for ARA Event FX..."
        : juce::String(map.documents) + " ARA document  |  " + juce::String(rows.size()) + " regions",
        juce::dontSendNotification);
    list.updateContent();
    list.repaint();
}

void HubEditor::paintListBoxItem(int index, juce::Graphics& g, int width, int height, bool selected)
{
    if (index < 0 || index >= static_cast<int>(rows.size()) || width <= 0 || height <= 0) return;
    const auto& row = rows[static_cast<size_t>(index)];
    g.fillAll(selected ? juce::Colour(0xff354554) : (index % 2 ? juce::Colour(0xff26313b) : juce::Colour(0xff222b33)));
    badge(g, {9, 7, 92, std::max(1, height - 14)}, statusColor(row.tag.status), arranger::label(row.tag.status));
    if (width < 160) return;
    badge(g, {109, 7, 46, std::max(1, height - 14)}, priorityColor(row.tag.priority), arranger::label(row.tag.priority));
    if (width < 340) return;
    g.setColour(juce::Colour(0xffdce4eb));
    g.setFont(juce::FontOptions(14.0f));
    g.drawFittedText(row.track, juce::Rectangle<int> {165, 0, std::min(150, width - 175), height}, juce::Justification::centredLeft, 1);
    if (width <= 490) return;
    const auto note = row.tag.valid && ! row.tag.note.empty() ? juce::String::fromUTF8(row.tag.note.c_str()) : row.name;
    g.drawFittedText(note, juce::Rectangle<int> {320, 0, width - 480, height}, juce::Justification::centredLeft, 1);
    g.setColour(juce::Colour(0xffaebdca));
    g.drawText(juce::String(row.start, 1) + " - " + juce::String(row.start + row.duration, 1) + " s",
               juce::Rectangle<int> {width - 155, 0, 145, height}, juce::Justification::centredRight);
}

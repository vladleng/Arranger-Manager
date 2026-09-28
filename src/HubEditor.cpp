#include "HubEditor.h"
#include <algorithm>

namespace
{
juce::Colour statusColor(arranger::Status s)
{
    switch (s)
    {
        case arranger::Status::pool: return juce::Colour(0xff8b78b8);
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
void badge(juce::Graphics& g, juce::Rectangle<int> bounds, juce::Colour color, const char* label)
{
    g.setColour(color);
    g.fillRoundedRectangle(bounds.toFloat(), 5.0f);
    g.setColour(juce::Colours::white);
    g.setFont(juce::FontOptions(13.0f, juce::Font::bold));
    g.drawText(label, bounds, juce::Justification::centred);
}
}

HubEditor::HubEditor(HubProcessor& p) : juce::AudioProcessorEditor(&p), processor(p)
{
    heading.setText("Arranger Manager Hub 0.0f", juce::dontSendNotification);
    heading.setFont(juce::FontOptions(21.0f, juce::Font::bold));
    addAndMakeVisible(heading);
    for (auto* label : { &path, &summary, &info, &notes, &help })
    {
        label->setFont(juce::FontOptions(14.0f));
        label->setColour(juce::Label::textColourId, juce::Colour(0xffdce4eb));
        addAndMakeVisible(*label);
    }
    help.setText("Saved project snapshot • Save in Studio Pro to update", juce::dontSendNotification);
    help.setColour(juce::Label::textColourId, juce::Colour(0xffaebdca));
    path.setText("No project selected", juce::dontSendNotification);
    summary.setText("Open the .song file of your current project", juce::dontSendNotification);
    choose.onClick = [this]
    {
        chooser = std::make_unique<juce::FileChooser>("Select a Studio Pro song", juce::File(), "*.song");
        chooser->launchAsync(juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
            [safe = juce::Component::SafePointer<HubEditor>(this)](const juce::FileChooser& selected)
            {
                if (safe == nullptr) return;
                const auto file = selected.getResult();
                if (file == juce::File()) return;
                safe->processor.setProjectPath(file.getFullPathName());
                safe->refreshSnapshot();
            });
    };
    refresh.onClick = [this] { refreshSnapshot(); };
    addAndMakeVisible(choose);
    addAndMakeVisible(refresh);
    list.setRowHeight(38);
    list.setColour(juce::ListBox::backgroundColourId, juce::Colour(0xff222b33));
    addAndMakeVisible(list);
    setResizable(true, true);
    setResizeLimits(460, 320, 1600, 1200);
    setSize(900, 560);
    timerCallback();
    startTimerHz(1);
}

HubEditor::~HubEditor() { stopTimer(); }
void HubEditor::paint(juce::Graphics& g) { g.fillAll(juce::Colour(0xff1d232b)); }

void HubEditor::resized()
{
    if (getWidth() < 400 || getHeight() < 250) return;
    auto area = getLocalBounds().reduced(16);
    auto header = area.removeFromTop(44);
    refresh.setBounds(header.removeFromRight(90).reduced(3));
    choose.setBounds(header.removeFromRight(130).reduced(3));
    heading.setBounds(header);
    path.setBounds(area.removeFromTop(28));
    summary.setBounds(area.removeFromTop(29));
    info.setBounds(area.removeFromTop(27));
    notes.setBounds(area.removeFromTop(27));
    help.setBounds(area.removeFromTop(28));
    area.removeFromTop(6);
    list.setBounds(area);
}

void HubEditor::timerCallback()
{
    const auto current = processor.getProjectPath();
    if (current.isEmpty()) return;
    const juce::File file(current);
    const auto modified = file.existsAsFile() ? file.getLastModificationTime().toMilliseconds() : -1;
    const auto size = file.existsAsFile() ? file.getSize() : -1;
    if (current != lastPath || modified != lastModified || size != lastSize) refreshSnapshot();
}

void HubEditor::refreshSnapshot()
{
    lastPath = processor.getProjectPath();
    rows.clear();
    if (lastPath.isEmpty())
    {
        path.setText("No project selected", juce::dontSendNotification);
        summary.setText("Open the .song file of your current project", juce::dontSendNotification);
        list.updateContent();
        list.repaint();
        return;
    }
    const juce::File file(lastPath);
    lastModified = file.existsAsFile() ? file.getLastModificationTime().toMilliseconds() : -1;
    lastSize = file.existsAsFile() ? file.getSize() : -1;
    path.setText(file.getFullPathName(), juce::dontSendNotification);
    const auto snapshot = arranger::readSongSnapshot(file);
    if (!snapshot.ok())
    {
        summary.setText("Cannot read project: " + snapshot.error, juce::dontSendNotification);
        info.setText({}, juce::dontSendNotification);
        notes.setText({}, juce::dontSendNotification);
    }
    else
    {
        int audio = 0, midi = 0;
        for (const auto& event : snapshot.events)
        {
            if (event.type == "AudioEvent") ++audio;
            if (event.type == "MusicPart") ++midi;
            rows.push_back({event, arranger::parse(event.name.toStdString())});
        }
        summary.setText(snapshot.documentTitle + "  |  " + juce::String(snapshot.trackCount) + " tracks  |  "
            + juce::String(audio) + " audio  |  " + juce::String(midi) + " MIDI", juce::dontSendNotification);
        info.setText("Info: " + snapshot.mediaTitle + (snapshot.artist.isNotEmpty() ? "  •  " + snapshot.artist : ""),
            juce::dontSendNotification);
        notes.setText("Notes: " + snapshot.notes.replaceCharacters("\r\n", "  "), juce::dontSendNotification);
    }
    list.updateContent();
    list.repaint();
}

void HubEditor::paintListBoxItem(int index, juce::Graphics& g, int width, int height, bool selected)
{
    if (index < 0 || index >= static_cast<int>(rows.size()) || width <= 0 || height <= 0) return;
    const auto& row = rows[static_cast<size_t>(index)];
    g.fillAll(selected ? juce::Colour(0xff354554) : (index % 2 ? juce::Colour(0xff26313b) : juce::Colour(0xff222b33)));
    const int h = std::max(1, height - 14);
    badge(g, {9, 7, 92, h}, statusColor(row.tag.status), arranger::label(row.tag.status));
    if (width < 160) return;
    badge(g, {109, 7, 46, h}, priorityColor(row.tag.priority), arranger::label(row.tag.priority));
    g.setColour(juce::Colour(0xffdce4eb));
    g.setFont(juce::FontOptions(14.0f));
    if (width < 300) return;
    const auto trackLabel = row.event.track + (row.event.type == "MusicPart" ? "  MIDI" : "  Audio");
    g.drawFittedText(trackLabel, {165, 0, 135, height}, juce::Justification::centredLeft, 1);
    const auto description = row.tag.valid && !row.tag.note.empty()
        ? juce::String::fromUTF8(row.tag.note.c_str()) : row.event.name;
    const auto showTime = width >= 700;
    g.drawFittedText(description, {305, 0, std::max(1, width - 315 - (showTime ? 155 : 0)), height},
        juce::Justification::centredLeft, 1);
    if (!showTime) return;
    g.setColour(juce::Colour(0xffaebdca));
    g.drawText("pos " + row.event.start + "  len " + row.event.length,
        {width - 155, 0, 145, height}, juce::Justification::centredRight);
}

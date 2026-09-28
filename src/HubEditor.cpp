#include "HubEditor.h"
#include "StudioProColour.h"
#include <algorithm>
#include <numeric>
#include <utility>

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
        case arranger::Status::wait: return juce::Colour(0xffd3a438);
        case arranger::Status::done: return juce::Colour(0xff34a878);
        case arranger::Status::blocked: return juce::Colour(0xffdd5b61);
        default: return juce::Colour(0xff56606a);
    }
}

void badge(juce::Graphics& g, juce::Rectangle<int> bounds, juce::Colour colour, const char* label)
{
    g.setColour(colour);
    g.fillRoundedRectangle(bounds.toFloat(), 5.0f);
    g.setColour(juce::Colours::white);
    g.setFont(juce::FontOptions(12.0f, juce::Font::bold));
    g.drawText(label, bounds, juce::Justification::centred);
}

struct Columns { int type, status, notes, notesWidth, position, nameWidth; bool showPosition, showTypeName; };
Columns columnsFor(int width)
{
    const bool wide = width >= 900;
    const bool showTypeName = width >= 700;
    const int nameWidth = std::max(175, static_cast<int>(width * (showTypeName ? 0.36f : 0.40f)));
    const int type = nameWidth + 8;
    const int status = type + (showTypeName ? 96 : 42);
    const int notes = status + 107;
    const int position = width - 135;
    return { type, status, notes, std::max(1, (wide ? position : width) - notes - 10), position,
        nameWidth, wide, showTypeName };
}

void mediaIcon(juce::Graphics& g, int x, int height, bool midi)
{
    const float left = static_cast<float>(x + 7);
    const float top = (static_cast<float>(height) - 18.0f) * 0.5f;
    g.setColour(midi ? juce::Colour(0xffed9844) : juce::Colour(0xff3b82f6));
    g.fillRoundedRectangle(left, top, 18.0f, 18.0f, 4.0f);
    g.setColour(juce::Colours::white);
    if (midi)
    {
        g.fillRect(left + 3.0f, top + 6.0f, 12.0f, 7.0f);
        g.setColour(juce::Colour(0xffed9844));
        for (int key = 1; key < 4; ++key)
            g.fillRect(left + 3.0f + 3.0f * static_cast<float>(key), top + 9.0f, 1.0f, 4.0f);
        g.fillRect(left + 5.0f, top + 6.0f, 2.0f, 4.0f);
        g.fillRect(left + 11.0f, top + 6.0f, 2.0f, 4.0f);
    }
    else
    {
        const float bars[] { 4.0f, 9.0f, 13.0f, 7.0f, 4.0f };
        for (int i = 0; i < 5; ++i)
            g.fillRoundedRectangle(left + 3.0f + 2.5f * static_cast<float>(i),
                top + (18.0f - bars[i]) * 0.5f, 1.5f, bars[i], 0.7f);
    }
}

void instrumentIcon(juce::Graphics& g, int x, int height)
{
    const float left = static_cast<float>(x + 7);
    const float top = (static_cast<float>(height) - 14.0f) * 0.5f;
    g.setColour(juce::Colour(0xffdce4eb));
    g.fillRoundedRectangle(left, top, 18.0f, 14.0f, 1.5f);
    g.setColour(juce::Colour(0xff26313b));
    for (int key = 1; key < 5; ++key)
        g.fillRect(left + 3.6f * static_cast<float>(key), top + 7.0f, 1.0f, 6.0f);
    for (const float key : { 3.0f, 6.6f, 13.8f })
        g.fillRect(left + key, top, 2.5f, 8.0f);
}

juce::Colour savedColour(const juce::String& saved)
{
    if (const auto argb = arranger::studioProColour(saved.toStdString()))
        return juce::Colour(static_cast<juce::uint32>(*argb));
    return juce::Colour(0xff586d83);
}

juce::String progress(const std::vector<arranger::SongEvent>& events)
{
    int tagged = 0, done = 0;
    for (const auto& event : events)
    {
        const auto tag = arranger::parse(event.name.toStdString());
        if (tag.valid) { ++tagged; if (tag.status == arranger::Status::done) ++done; }
    }
    return tagged == 0 ? juce::String("-") : juce::String(done) + "/" + juce::String(tagged) + " DONE";
}

template <typename Items>
std::vector<size_t> orderByStart(const Items& items)
{
    std::vector<size_t> order(items.size());
    std::iota(order.begin(), order.end(), size_t{0});
    std::stable_sort(order.begin(), order.end(), [&](size_t a, size_t b)
    {
        return items[a].start.getDoubleValue() < items[b].start.getDoubleValue();
    });
    return order;
}
}

HubEditor::HubEditor(std::function<juce::String()> getPath,
    std::function<void(juce::String)> setPath)
    : getProjectPath(std::move(getPath)), setProjectPath(std::move(setPath))
{
    heading.setText("Arranger Manager 0.0l", juce::dontSendNotification);
    heading.setFont(juce::FontOptions(21.0f, juce::Font::bold));
    addAndMakeVisible(heading);
    for (auto* label : { &path, &summary, &info, &notes, &help })
    {
        label->setFont(juce::FontOptions(14.0f));
        label->setColour(juce::Label::textColourId, juce::Colour(0xffdce4eb));
        addAndMakeVisible(*label);
    }
    help.setText("Saved project snapshot - Save in Studio Pro to update", juce::dontSendNotification);
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
                safe->setProjectPath(file.getFullPathName());
                safe->refreshSnapshot();
            });
    };
    refresh.onClick = [this] { refreshSnapshot(); };
    addAndMakeVisible(choose);
    addAndMakeVisible(refresh);
    list.setRowHeight(36);
    list.setColour(juce::ListBox::backgroundColourId, juce::Colour(0xff222b33));
    addAndMakeVisible(list);
    setSize(980, 620);
    timerCallback();
    startTimerHz(1);
}

HubEditor::~HubEditor() { stopTimer(); }

void HubEditor::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff1d232b));
    if (list.getWidth() == 0) return;
    const auto header = juce::Rectangle<int> {list.getX(), list.getY() - 27, list.getWidth(), 26};
    g.setColour(juce::Colour(0xff2a343e));
    g.fillRect(header);
    g.setColour(juce::Colour(0xffaebdca));
    g.setFont(juce::FontOptions(12.0f, juce::Font::bold));
    const auto layout = columnsFor(list.getWidth());
    g.drawText("NAME", juce::Rectangle<int> {header.getX() + 12, header.getY(), layout.nameWidth - 12, header.getHeight()},
        juce::Justification::centredLeft);
    g.drawText("TYPE", juce::Rectangle<int> {header.getX() + layout.type, header.getY(), 40, header.getHeight()},
        juce::Justification::centredLeft);
    g.drawText("STATUS", juce::Rectangle<int> {header.getX() + layout.status, header.getY(), 100, header.getHeight()},
        juce::Justification::centredLeft);
    g.drawText("NOTES", juce::Rectangle<int> {header.getX() + layout.notes, header.getY(), layout.notesWidth, header.getHeight()},
        juce::Justification::centredLeft);
    if (layout.showPosition)
        g.drawText("POSITION", juce::Rectangle<int> {header.getX() + layout.position, header.getY(), 130, header.getHeight()},
            juce::Justification::centredLeft);
}

void HubEditor::resized()
{
    if (getWidth() < 400 || getHeight() < 250) return;
    auto area = getLocalBounds().reduced(16);
    auto header = area.removeFromTop(44);
    refresh.setBounds(header.removeFromRight(90).reduced(3));
    choose.setBounds(header.removeFromRight(130).reduced(3));
    heading.setBounds(header);
    path.setBounds(area.removeFromTop(25));
    summary.setBounds(area.removeFromTop(27));
    info.setBounds(area.removeFromTop(25));
    notes.setBounds(area.removeFromTop(25));
    help.setBounds(area.removeFromTop(23));
    area.removeFromTop(32);
    list.setBounds(area);
    repaint();
}

void HubEditor::timerCallback()
{
    const auto current = getProjectPath();
    if (current.isEmpty()) return;
    const juce::File file(current);
    const auto modified = file.existsAsFile() ? file.getLastModificationTime().toMilliseconds() : -1;
    const auto size = file.existsAsFile() ? file.getSize() : -1;
    if (current != lastPath || modified != lastModified || size != lastSize) refreshSnapshot();
}

void HubEditor::refreshSnapshot()
{
    const auto current = getProjectPath();
    if (current != lastPath) collapsed.clear();
    lastPath = current;
    snapshot = {};
    rows.clear();
    if (lastPath.isEmpty())
    {
        path.setText("No project selected", juce::dontSendNotification);
        summary.setText("Open the .song file of your current project", juce::dontSendNotification);
        info.setText({}, juce::dontSendNotification);
        notes.setText({}, juce::dontSendNotification);
        list.updateContent();
        return;
    }
    const juce::File file(lastPath);
    lastModified = file.existsAsFile() ? file.getLastModificationTime().toMilliseconds() : -1;
    lastSize = file.existsAsFile() ? file.getSize() : -1;
    path.setText(file.getFullPathName(), juce::dontSendNotification);
    snapshot = arranger::readSongSnapshot(file);
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
        }
        summary.setText(snapshot.documentTitle + "  |  " + juce::String(snapshot.trackCount) + " tracks  |  "
            + juce::String(audio) + " audio  |  " + juce::String(midi) + " MIDI", juce::dontSendNotification);
        info.setText("Info: " + snapshot.mediaTitle + (snapshot.artist.isNotEmpty() ? "  -  " + snapshot.artist : ""),
            juce::dontSendNotification);
        notes.setText("Notes: " + snapshot.notes.replaceCharacters("\r\n", "  "), juce::dontSendNotification);
        rebuildRows();
    }
    list.updateContent();
    list.repaint();
}

void HubEditor::rebuildRows()
{
    rows.clear();
    if (!snapshot.ok()) return;
    auto addClip = [this](const arranger::SongEvent& event, int depth)
    {
        const auto tag = arranger::parse(event.name.toStdString());
        Row row { RowKind::clip, {},
            {}, "pos " + event.start + "  len " + event.length,
            {}, tag, {}, depth };
        row.notes = tag.valid ? juce::String::fromUTF8(tag.note.c_str()) : event.name.trim();
        row.midi = event.type == "MusicPart";
        for (auto index : arranger::matchingSectionIndices(snapshot, event))
        {
            const auto& section = snapshot.sections[index];
            row.sections.push_back({section.name, savedColour(section.color)});
            const auto prefix = "[" + section.name + "]";
            if (row.notes.startsWith(prefix)) row.notes = row.notes.substring(prefix.length()).trimStart();
        }
        if (row.sections.empty()) row.title = "Clip";
        rows.push_back(std::move(row));
    };

    Row project {RowKind::project, snapshot.documentTitle.isNotEmpty() ? snapshot.documentTitle : juce::String("Project"),
        juce::String(static_cast<int>(snapshot.tracks.size())) + " tracks / "
            + juce::String(static_cast<int>(snapshot.events.size())) + " clips",
        {}, progress(snapshot.events), {}, "project", 0, true, !collapsed.contains("project")};
    rows.push_back(std::move(project));
    if (collapsed.contains("project")) return;

    rows.push_back({RowKind::group, "Tracks", juce::String(static_cast<int>(snapshot.tracks.size())) + " tracks", {}, {}, {},
        "tracks", 1, true, !collapsed.contains("tracks")});
    if (!collapsed.contains("tracks"))
    {
        for (size_t i = 0; i < snapshot.tracks.size(); ++i)
        {
            const auto& track = snapshot.tracks[i];
            const std::string key = "track:" + (track.id.isNotEmpty() ? track.id.toStdString() : std::to_string(i));
            rows.push_back({RowKind::track, track.name,
                juce::String(static_cast<int>(track.events.size())) + " clips", {}, progress(track.events), {},
                key, 2, !track.events.empty(), !collapsed.contains(key)});
            rows.back().notes = track.notes.replaceCharacters("\r\n", "  ").trim();
            rows.back().trackColour = savedColour(track.color);
            rows.back().mediaType = track.mediaType;
            if (collapsed.contains(key)) continue;
            for (auto index : orderByStart(track.events)) addClip(track.events[index], 3);
        }
    }

    rows.push_back({RowKind::group, "Markers", juce::String(static_cast<int>(snapshot.markers.size())) + " markers", {}, {}, {},
        "markers", 1, true, !collapsed.contains("markers")});
    if (!collapsed.contains("markers"))
        for (auto index : orderByStart(snapshot.markers))
        {
            const auto& marker = snapshot.markers[index];
            rows.push_back({RowKind::marker, marker.name, {}, "pos " + marker.start, {}, {}, {}, 2});
        }
}

void HubEditor::listBoxItemClicked(int index, const juce::MouseEvent&)
{
    if (index < 0 || index >= static_cast<int>(rows.size())) return;
    const auto& row = rows[static_cast<size_t>(index)];
    if (!row.expandable) return;
    const auto key = row.key;
    if (!collapsed.erase(key)) collapsed.insert(key);
    rebuildRows();
    list.updateContent();
    list.repaint();
}

void HubEditor::paintListBoxItem(int index, juce::Graphics& g, int width, int height, bool selected)
{
    if (index < 0 || index >= static_cast<int>(rows.size()) || width <= 0 || height <= 0) return;
    const auto& row = rows[static_cast<size_t>(index)];
    const bool parent = row.kind == RowKind::project || row.kind == RowKind::group;
    g.fillAll(selected ? juce::Colour(0xff354554)
        : parent ? juce::Colour(0xff2b3540)
        : (index % 2 ? juce::Colour(0xff26313b) : juce::Colour(0xff222b33)));
    g.setColour(juce::Colour(0xff39434d));
    g.fillRect(0, height - 1, width, 1);
    const auto layout = columnsFor(width);
    const int x = 10 + row.depth * 19;
    if (row.expandable)
    {
        g.setColour(juce::Colour(0xffaebdca));
        g.setFont(juce::FontOptions(13.0f));
        g.drawText(row.expanded ? "v" : ">", juce::Rectangle<int> {x, 0, 15, height}, juce::Justification::centred);
    }
    // Keep the arrow slot on tracks without clips so all track-color dots align.
    const int textX = x + (row.expandable || row.kind == RowKind::track ? 18 : 9);
    g.setColour(parent ? juce::Colours::white : juce::Colour(0xffdce4eb));
    g.setFont(juce::FontOptions(14.0f, parent ? juce::Font::bold : juce::Font::plain));
    int titleX = textX;
    if (row.kind == RowKind::track)
    {
        const auto circle = juce::Rectangle<float> {static_cast<float>(titleX), (height - 12.0f) * 0.5f, 12.0f, 12.0f};
        g.setColour(row.trackColour);
        g.fillEllipse(circle);
        g.setColour(juce::Colours::white.withAlpha(0.45f));
        g.drawEllipse(circle, 1.0f);
        titleX += 19;
    }
    for (const auto& section : row.sections)
    {
        const auto label = "[" + section.name + "]";
        const int available = layout.nameWidth - titleX - 5;
        if (available < 35) break;
        const auto circle = juce::Rectangle<float> {static_cast<float>(titleX), (height - 12.0f) * 0.5f, 12.0f, 12.0f};
        g.setColour(section.colour);
        g.fillEllipse(circle);
        g.setColour(juce::Colours::white.withAlpha(0.45f));
        g.drawEllipse(circle, 1.0f);
        titleX += 17;
        const int labelWidth = std::min(available - 17, std::min(130, static_cast<int>(label.length()) * 8 + 4));
        g.setColour(juce::Colour(0xffdce4eb));
        g.drawText(label, juce::Rectangle<int> {titleX, 0, labelWidth, height}, juce::Justification::centredLeft, true);
        titleX += labelWidth + 7;
    }
    g.setColour(parent ? juce::Colours::white : juce::Colour(0xffdce4eb));
    const auto title = row.title + (row.detail.isNotEmpty() ? "   " + row.detail : "");
    g.drawFittedText(title, juce::Rectangle<int> {titleX, 0, std::max(1, layout.nameWidth - titleX), height},
        juce::Justification::centredLeft, 1);

    if (row.kind == RowKind::clip)
    {
        mediaIcon(g, layout.type, height, row.midi);
        if (row.tag.valid)
            badge(g, {layout.status, 7, 90, height - 14}, statusColor(row.tag.status), arranger::label(row.tag.status));
    }
    else if (row.kind == RowKind::track)
    {
        if (row.mediaType.equalsIgnoreCase("Audio")) mediaIcon(g, layout.type, height, false);
        else if (row.mediaType.equalsIgnoreCase("Music")) instrumentIcon(g, layout.type, height);
        if (layout.showTypeName)
        {
            g.setColour(juce::Colour(0xffdce4eb));
            g.setFont(juce::FontOptions(12.0f));
            g.drawText(row.mediaType,
                juce::Rectangle<int> {layout.type + 32, 0, layout.status - layout.type - 34, height},
                juce::Justification::centredLeft, true);
        }
    }
    if (row.kind != RowKind::clip && row.summary.isNotEmpty())
    {
        g.setColour(juce::Colour(0xffaebdca));
        g.setFont(juce::FontOptions(12.0f));
        g.drawText(row.summary, juce::Rectangle<int> {layout.status, 0, 105, height}, juce::Justification::centredLeft);
    }
    if (row.notes.isNotEmpty())
    {
        g.setColour(juce::Colour(0xffdce4eb));
        g.setFont(juce::FontOptions(12.0f));
        g.drawText(row.notes, juce::Rectangle<int> {layout.notes, 0, layout.notesWidth, height},
            juce::Justification::centredLeft, true);
    }
    if (layout.showPosition && row.position.isNotEmpty())
    {
        g.setColour(juce::Colour(0xffaebdca));
        g.setFont(juce::FontOptions(12.0f));
        g.drawFittedText(row.position, juce::Rectangle<int> {layout.position, 0, 125, height},
            juce::Justification::centredLeft, 1);
    }
}

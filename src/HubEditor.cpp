#include "HubEditor.h"
#include "StudioProColour.h"
#include <algorithm>
#include <iterator>
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

struct Columns { int type, status, prog, notes, notesWidth, position, nameWidth, statusWidth, progWidth;
    bool showPosition, showTypeName; };
Columns columnsFor(int width)
{
    const bool wide = width >= 1050;
    const bool showTypeName = width >= 700;
    const int nameWidth = std::max(145, static_cast<int>(width * (showTypeName ? 0.34f : 0.35f)));
    const int type = nameWidth + 6;
    const int status = type + (showTypeName ? 92 : 32);
    const int statusWidth = showTypeName ? 90 : 76;
    const int prog = status + (showTypeName ? 97 : 82);
    const int progWidth = showTypeName ? 86 : 64;
    const int notes = prog + (showTypeName ? 96 : 74);
    const int position = width - 135;
    return { type, status, prog, notes, std::max(1, (wide ? position : width) - notes - 10), position,
        nameWidth, statusWidth, progWidth, wide, showTypeName };
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

std::pair<int, int> progressCounts(const std::vector<arranger::SongEvent>& events)
{
    int tagged = 0, done = 0;
    for (const auto& event : events)
    {
        const auto tag = arranger::parse(event.name.toStdString());
        if (tag.valid) { ++tagged; if (tag.status == arranger::Status::done) ++done; }
    }
    return {done, tagged};
}

void progressBar(juce::Graphics& g, int x, int height, int width, int done, int total)
{
    g.setColour(juce::Colour(0xffaebdca));
    g.setFont(juce::FontOptions(11.0f));
    if (total == 0)
    {
        g.drawText("-", juce::Rectangle<int> {x, 0, width, height}, juce::Justification::centredLeft);
        return;
    }
    g.drawText(juce::String(done) + "/" + juce::String(total), juce::Rectangle<int> {x, 1, width, 17},
        juce::Justification::centredLeft);
    const auto bar = juce::Rectangle<float> {static_cast<float>(x), static_cast<float>(height - 12),
        static_cast<float>(width), 5.0f};
    g.setColour(juce::Colour(0xff49545e));
    g.fillRoundedRectangle(bar, 2.5f);
    if (done > 0)
    {
        g.setColour(juce::Colour(0xff34a878));
        g.fillRoundedRectangle(bar.withWidth(bar.getWidth() * static_cast<float>(done) / static_cast<float>(total)), 2.5f);
    }
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
    std::function<void(juce::String)> setPath, arranger::SongCatalog* songCatalog,
    std::function<void()> onSaveCatalog)
    : getProjectPath(std::move(getPath)), setProjectPath(std::move(setPath)),
      catalog(songCatalog), saveCatalog(std::move(onSaveCatalog))
{
    heading.setText("Arranger Manager 0.0m", juce::dontSendNotification);
    heading.setFont(juce::FontOptions(21.0f, juce::Font::bold));
    addAndMakeVisible(heading);
    for (auto* label : { &path, &summary, &info, &notes, &help })
    {
        label->setFont(juce::FontOptions(14.0f));
        label->setColour(juce::Label::textColourId, juce::Colour(0xffdce4eb));
        addAndMakeVisible(*label);
    }
    help.setText(catalog != nullptr ? "Select a folder, then Add .song. Right-click a song to move it."
        : "Saved project snapshot - Save in Studio Pro to update", juce::dontSendNotification);
    help.setColour(juce::Label::textColourId, juce::Colour(0xffaebdca));
    path.setText("No project selected", juce::dontSendNotification);
    summary.setText("Open the .song file of your current project", juce::dontSendNotification);
    if (catalog != nullptr) choose.setButtonText("Add .song");
    choose.onClick = [this]
    {
        chooser = std::make_unique<juce::FileChooser>("Select a Studio Pro song", juce::File(), "*.song");
        chooser->launchAsync(juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles
            | (catalog != nullptr ? juce::FileBrowserComponent::canSelectMultipleItems : 0),
            [safe = juce::Component::SafePointer<HubEditor>(this)](const juce::FileChooser& selected)
            {
                if (safe == nullptr) return;
                if (safe->catalog != nullptr)
                {
                    juce::String lastAdded;
                    for (const auto& file : selected.getResults())
                        if (safe->catalog->addSong(file.getFullPathName(), safe->selectedFolderId))
                            lastAdded = file.getFullPathName();
                    if (lastAdded.isNotEmpty())
                    {
                        safe->setProjectPath(lastAdded);
                        safe->expandedSongs.insert("project:" + lastAdded.toStdString());
                        safe->catalogChanged();
                    }
                }
                else
                {
                    const auto file = selected.getResult();
                    if (file == juce::File()) return;
                    safe->setProjectPath(file.getFullPathName());
                }
                safe->refreshSnapshot();
            });
    };
    newFolder.onClick = [this] { promptNewFolder(); };
    if (catalog != nullptr) addAndMakeVisible(newFolder);
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
    g.drawText("STATUS", juce::Rectangle<int> {header.getX() + layout.status, header.getY(), layout.statusWidth, header.getHeight()},
        juce::Justification::centredLeft);
    g.drawText("PROG", juce::Rectangle<int> {header.getX() + layout.prog, header.getY(), layout.progWidth, header.getHeight()},
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
    const bool compact = catalog != nullptr && getWidth() < 760;
    auto header = area.removeFromTop(compact ? 80 : 44);
    if (compact)
    {
        heading.setBounds(header.removeFromTop(36));
        refresh.setBounds(header.removeFromRight(90).reduced(3));
        choose.setBounds(header.removeFromRight(130).reduced(3));
        newFolder.setBounds(header.removeFromRight(110).reduced(3));
    }
    else
    {
        refresh.setBounds(header.removeFromRight(90).reduced(3));
        choose.setBounds(header.removeFromRight(130).reduced(3));
        if (catalog != nullptr) newFolder.setBounds(header.removeFromRight(110).reduced(3));
        heading.setBounds(header);
    }
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
    if (catalog != nullptr)
    {
        if (lastPath != getProjectPath() || songCache.size() != catalog->songs.size())
        {
            refreshSnapshot();
            return;
        }
        for (const auto& song : catalog->songs)
        {
            const juce::File file(song.path);
            const auto key = song.path.toStdString();
            const auto found = songCache.find(key);
            if (found == songCache.end() || found->second.modified != (file.existsAsFile() ? file.getLastModificationTime().toMilliseconds() : -1)
                || found->second.size != (file.existsAsFile() ? file.getSize() : -1))
            {
                refreshSnapshot();
                return;
            }
        }
        return;
    }
    const auto current = getProjectPath();
    if (current.isEmpty()) return;
    const juce::File file(current);
    const auto modified = file.existsAsFile() ? file.getLastModificationTime().toMilliseconds() : -1;
    const auto size = file.existsAsFile() ? file.getSize() : -1;
    if (current != lastPath || modified != lastModified || size != lastSize) refreshSnapshot();
}

void HubEditor::refreshSnapshot()
{
    if (catalog != nullptr) { refreshCatalog(); return; }
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

void HubEditor::refreshCatalog()
{
    lastPath = getProjectPath();
    auto targetName = juce::String("Songs");
    for (const auto& folder : catalog->folders)
        if (folder.id == selectedFolderId) { targetName = folder.name; break; }
    help.setText("Add .song to: " + targetName + "  |  Right-click a song to move it",
        juce::dontSendNotification);
    std::set<std::string> paths;
    for (const auto& song : catalog->songs)
    {
        const auto key = song.path.toStdString();
        paths.insert(key);
        const juce::File file(song.path);
        const auto modified = file.existsAsFile() ? file.getLastModificationTime().toMilliseconds() : -1;
        const auto size = file.existsAsFile() ? file.getSize() : -1;
        auto found = songCache.find(key);
        if (found == songCache.end() || found->second.modified != modified || found->second.size != size)
            songCache[key] = {arranger::readSongSnapshot(file), modified, size};
    }
    for (auto it = songCache.begin(); it != songCache.end();)
        it = paths.contains(it->first) ? std::next(it) : songCache.erase(it);

    snapshot = {};
    if (const auto found = songCache.find(lastPath.toStdString()); found != songCache.end())
        snapshot = found->second.snapshot;
    updateHeader(lastPath);
    rebuildRows();
    list.updateContent();
    list.repaint();
}

void HubEditor::updateHeader(const juce::String& selectedPath)
{
    if (selectedPath.isEmpty() || !songCache.contains(selectedPath.toStdString()))
    {
        path.setText("Song catalog", juce::dontSendNotification);
        summary.setText(juce::String(static_cast<int>(catalog->songs.size())) + " songs  |  "
            + juce::String(static_cast<int>(catalog->folders.size())) + " folders", juce::dontSendNotification);
        info.setText("Add a saved .song file to begin", juce::dontSendNotification);
        notes.setText({}, juce::dontSendNotification);
        return;
    }
    path.setText(selectedPath, juce::dontSendNotification);
    if (!snapshot.ok())
    {
        summary.setText("Cannot read project: " + snapshot.error, juce::dontSendNotification);
        info.setText({}, juce::dontSendNotification);
        notes.setText({}, juce::dontSendNotification);
        return;
    }
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
}

void HubEditor::catalogChanged()
{
    if (saveCatalog) saveCatalog();
    refreshCatalog();
}

void HubEditor::rebuildRows()
{
    rows.clear();
    if (catalog == nullptr)
    {
        if (snapshot.ok()) appendSongRows(snapshot, 0, "single", lastPath, false);
        return;
    }

    auto addFolder = [this](const juce::String& id, const juce::String& name)
    {
        const auto key = "folder:" + id.toStdString();
        int count = 0, done = 0, total = 0;
        for (const auto& song : catalog->songs)
            if (song.folderId == id)
            {
                ++count;
                if (const auto found = songCache.find(song.path.toStdString()); found != songCache.end())
                {
                    const auto [d, t] = progressCounts(found->second.snapshot.events);
                    done += d;
                    total += t;
                }
            }
        Row folder {RowKind::folder, name, juce::String(count) + " songs", {}, {}, {}, key,
            0, true, !collapsed.contains(key)};
        folder.folderId = id;
        folder.done = done;
        folder.total = total;
        rows.push_back(std::move(folder));
        if (collapsed.contains(key)) return;
        for (const auto& song : catalog->songs)
            if (song.folderId == id)
            {
                const auto found = songCache.find(song.path.toStdString());
                if (found != songCache.end())
                    appendSongRows(found->second.snapshot, 1, song.path.toStdString(), song.path, true);
            }
    };
    addFolder({}, "Songs");
    for (const auto& folder : catalog->folders) addFolder(folder.id, folder.name);
}

void HubEditor::appendSongRows(const arranger::SongSnapshot& song, int depth,
    const std::string& key, const juce::String& songPath, bool catalogProject)
{
    const auto projectKey = catalogProject ? "project:" + key : std::string("project");
    const bool expanded = catalogProject ? expandedSongs.contains(projectKey) : !collapsed.contains(projectKey);
    const auto title = song.documentTitle.isNotEmpty() ? song.documentTitle
        : songPath.isNotEmpty() ? juce::File(songPath).getFileNameWithoutExtension() : juce::String("Project");
    Row project {RowKind::project, title,
        song.ok() ? juce::String(static_cast<int>(song.tracks.size())) + " tracks / "
            + juce::String(static_cast<int>(song.events.size())) + " clips" : song.error,
        {}, {}, {}, projectKey, depth, song.ok(), expanded};
    const auto [projectDone, projectTotal] = progressCounts(song.events);
    project.done = projectDone;
    project.total = projectTotal;
    project.songPath = songPath;
    rows.push_back(std::move(project));
    if (!song.ok() || !expanded) return;

    auto addClip = [this, &song, &songPath](const arranger::SongEvent& event, int clipDepth)
    {
        const auto tag = arranger::parse(event.name.toStdString());
        Row row { RowKind::clip, {},
            {}, "pos " + event.start + "  len " + event.length,
            {}, tag, {}, clipDepth };
        row.notes = tag.valid ? juce::String::fromUTF8(tag.note.c_str()) : event.name.trim();
        row.midi = event.type == "MusicPart";
        row.songPath = songPath;
        for (auto index : arranger::matchingSectionIndices(song, event))
        {
            const auto& section = song.sections[index];
            row.sections.push_back({section.name, savedColour(section.color)});
            const auto prefix = "[" + section.name + "]";
            if (row.notes.startsWith(prefix)) row.notes = row.notes.substring(prefix.length()).trimStart();
        }
        if (row.sections.empty()) row.title = "Clip";
        rows.push_back(std::move(row));
    };

    const auto tracksKey = catalogProject ? key + ":tracks" : std::string("tracks");
    rows.push_back({RowKind::group, "Tracks", juce::String(static_cast<int>(song.tracks.size())) + " tracks", {}, {}, {},
        tracksKey, depth + 1, true, !collapsed.contains(tracksKey)});
    if (!collapsed.contains(tracksKey))
    {
        for (size_t i = 0; i < song.tracks.size(); ++i)
        {
            const auto& track = song.tracks[i];
            const std::string trackKey = (catalogProject ? key + ":" : std::string()) + "track:"
                + (track.id.isNotEmpty() ? track.id.toStdString() : std::to_string(i));
            rows.push_back({RowKind::track, track.name,
                juce::String(static_cast<int>(track.events.size())) + " clips", {}, {}, {},
                trackKey, depth + 2, !track.events.empty(), !collapsed.contains(trackKey)});
            rows.back().notes = track.notes.replaceCharacters("\r\n", "  ").trim();
            rows.back().trackColour = savedColour(track.color);
            rows.back().mediaType = track.mediaType;
            rows.back().songPath = songPath;
            const auto [done, total] = progressCounts(track.events);
            rows.back().done = done;
            rows.back().total = total;
            if (collapsed.contains(trackKey)) continue;
            for (auto index : orderByStart(track.events)) addClip(track.events[index], depth + 3);
        }
    }

    const auto markersKey = catalogProject ? key + ":markers" : std::string("markers");
    rows.push_back({RowKind::group, "Markers", juce::String(static_cast<int>(song.markers.size())) + " markers", {}, {}, {},
        markersKey, depth + 1, true, !collapsed.contains(markersKey)});
    if (!collapsed.contains(markersKey))
        for (auto index : orderByStart(song.markers))
        {
            const auto& marker = song.markers[index];
            rows.push_back({RowKind::marker, marker.name, {}, "pos " + marker.start, {}, {}, {}, depth + 2});
            rows.back().songPath = songPath;
        }
}

void HubEditor::promptNewFolder()
{
    if (catalog == nullptr) return;
    auto* dialog = new juce::AlertWindow("New folder", "Name for the song group",
        juce::MessageBoxIconType::QuestionIcon);
    dialog->addTextEditor("name", {}, "Folder name");
    dialog->addButton("Create", 1, juce::KeyPress(juce::KeyPress::returnKey));
    dialog->addButton("Cancel", 0, juce::KeyPress(juce::KeyPress::escapeKey));
    dialog->enterModalState(true, juce::ModalCallbackFunction::create(
        [safe = juce::Component::SafePointer<HubEditor>(this), dialog](int result)
        {
            if (safe == nullptr || result != 1) return;
            const auto id = safe->catalog->addFolder(dialog->getTextEditorContents("name"));
            if (id.isNotEmpty())
            {
                safe->selectedFolderId = id;
                safe->catalogChanged();
            }
        }), true);
}

void HubEditor::showSongMenu(const juce::String& songPath)
{
    juce::PopupMenu menu;
    menu.addItem(1, "Move to Songs (root)");
    for (size_t i = 0; i < catalog->folders.size(); ++i)
        menu.addItem(static_cast<int>(i) + 10, "Move to " + catalog->folders[i].name);
    menu.addSeparator();
    menu.addItem(1000, "Remove from catalog");
    menu.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(&list),
        [safe = juce::Component::SafePointer<HubEditor>(this), songPath](int result)
        {
            if (safe == nullptr || result == 0) return;
            if (result == 1000)
            {
                safe->catalog->removeSong(songPath);
                if (safe->getProjectPath().equalsIgnoreCase(songPath))
                    safe->setProjectPath(safe->catalog->songs.empty() ? juce::String() : safe->catalog->songs.front().path);
            }
            else
            {
                const auto folderId = result == 1 ? juce::String()
                    : (result >= 10 && static_cast<size_t>(result - 10) < safe->catalog->folders.size()
                        ? safe->catalog->folders[static_cast<size_t>(result - 10)].id : juce::String());
                if (result != 1 && folderId.isEmpty()) return;
                safe->catalog->moveSong(songPath, folderId);
                safe->selectedFolderId = folderId;
                safe->collapsed.erase("folder:" + folderId.toStdString());
            }
            safe->catalogChanged();
        });
}

void HubEditor::showFolderMenu(const juce::String& folderId)
{
    if (folderId.isEmpty()) return;
    juce::PopupMenu menu;
    menu.addItem(1, "Remove folder (keep songs)");
    menu.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(&list),
        [safe = juce::Component::SafePointer<HubEditor>(this), folderId](int result)
        {
            if (safe == nullptr || result != 1) return;
            safe->catalog->removeFolder(folderId);
            if (safe->selectedFolderId == folderId) safe->selectedFolderId.clear();
            safe->catalogChanged();
        });
}

void HubEditor::listBoxItemClicked(int index, const juce::MouseEvent& event)
{
    if (index < 0 || index >= static_cast<int>(rows.size())) return;
    const auto& row = rows[static_cast<size_t>(index)];
    if (catalog != nullptr && event.mods.isPopupMenu())
    {
        if (row.kind == RowKind::project) showSongMenu(row.songPath);
        else if (row.kind == RowKind::folder) showFolderMenu(row.folderId);
        return;
    }
    const auto key = row.key;
    if (catalog != nullptr)
    {
        if (row.kind == RowKind::folder)
        {
            selectedFolderId = row.folderId;
            auto targetName = juce::String("Songs");
            for (const auto& folder : catalog->folders)
                if (folder.id == selectedFolderId) { targetName = folder.name; break; }
            help.setText("Add .song to: " + targetName + "  |  Right-click a song to move it",
                juce::dontSendNotification);
        }
        if (row.kind == RowKind::project)
        {
            const auto selectedPath = row.songPath;
            if (getProjectPath() != selectedPath) setProjectPath(selectedPath);
            if (!expandedSongs.erase(key)) expandedSongs.insert(key);
            refreshCatalog();
            return;
        }
        if (row.songPath.isNotEmpty() && getProjectPath() != row.songPath)
        {
            const auto selectedPath = row.songPath;
            setProjectPath(selectedPath);
            refreshCatalog();
            return;
        }
    }
    if (!row.expandable) return;
    if (!collapsed.erase(key)) collapsed.insert(key);
    rebuildRows();
    list.updateContent();
    list.repaint();
}

void HubEditor::paintListBoxItem(int index, juce::Graphics& g, int width, int height, bool selected)
{
    if (index < 0 || index >= static_cast<int>(rows.size()) || width <= 0 || height <= 0) return;
    const auto& row = rows[static_cast<size_t>(index)];
    const bool parent = row.kind == RowKind::folder || row.kind == RowKind::project || row.kind == RowKind::group;
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
    if (row.kind == RowKind::folder)
    {
        g.setColour(juce::Colour(0xffd3a438));
        g.fillRoundedRectangle(static_cast<float>(titleX), (height - 11.0f) * 0.5f,
            15.0f, 11.0f, 2.0f);
        g.fillRect(titleX + 1, (height - 15) / 2, 7, 4);
        titleX += 22;
    }
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
            badge(g, {layout.status, 7, layout.statusWidth, height - 14},
                statusColor(row.tag.status), arranger::label(row.tag.status));
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
    if (row.kind == RowKind::folder || row.kind == RowKind::project || row.kind == RowKind::track)
        progressBar(g, layout.prog, height, layout.progWidth, row.done, row.total);
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

#include "SongSnapshot.h"
#include <algorithm>
#include <map>
#include <numeric>
#include <optional>

namespace arranger
{
namespace
{
std::optional<MarkerNote> parseMarkerNote(juce::String name)
{
    name = name.trim();
    if (!name.startsWithIgnoreCase("mk")) return std::nullopt;
    int index = 2;
    while (index < name.length() && juce::CharacterFunctions::isDigit(name[index])) ++index;
    if (index == 2 || name.substring(2, index).getIntValue() < 1) return std::nullopt;
    if (index < name.length() && name[index] != ' ' && name[index] != '\t'
        && name[index] != '|' && name[index] != ':') return std::nullopt;
    auto text = name.substring(index).trim();
    if (text.startsWithChar('|') || text.startsWithChar(':')) text = text.substring(1).trim();
    if (text.isEmpty()) return std::nullopt;
    return MarkerNote {"mk" + name.substring(2, index), text};
}

juce::String readEntry(juce::ZipFile& zip, const juce::String& name, juce::int64 limit, juce::String& error)
{
    const auto index = zip.getIndexOfFileName(name);
    if (index < 0) { error = "Missing " + name; return {}; }
    const auto* entry = zip.getEntry(index);
    if (entry == nullptr || entry->uncompressedSize < 0 || entry->uncompressedSize > limit)
    {
        error = "Oversized or invalid " + name;
        return {};
    }
    std::unique_ptr<juce::InputStream> stream(zip.createStreamForEntry(index));
    if (stream == nullptr) { error = "Cannot read " + name; return {}; }
    juce::MemoryBlock data;
    stream->readIntoMemoryBlock(data, static_cast<int>(limit + 1));
    if (data.getSize() > static_cast<size_t>(limit) || data.getSize() != static_cast<size_t>(entry->uncompressedSize))
    {
        error = "Incomplete or oversized " + name;
        return {};
    }
    return juce::String::fromUTF8(static_cast<const char*>(data.getData()), static_cast<int>(data.getSize()));
}

std::unique_ptr<juce::XmlElement> parse(juce::String xml, const juce::String& name, juce::String& error)
{
    // String-based XmlDocument parsing does not discard a UTF-8 byte-order mark.
    if (xml.startsWithChar(static_cast<juce::juce_wchar>(0xfeff))) xml = xml.substring(1);
    if (xml.containsIgnoreCase("<!DOCTYPE") || xml.containsIgnoreCase("<!ENTITY"))
    {
        error = "Unsupported XML in " + name;
        return {};
    }
    // Studio Pro 8.1.2 writes x:id without an XML namespace declaration.
    xml = xml.replace(" x:id=", " x_id=");
    juce::XmlDocument document(xml);
    auto root = document.getDocumentElement();
    if (root == nullptr) error = "Cannot parse " + name + ": " + document.getLastParseError();
    return root;
}
}

SongSnapshot readSongSnapshot(const juce::File& song)
{
    SongSnapshot snapshot;
    if (!song.existsAsFile() || !song.hasFileExtension("song"))
    {
        snapshot.error = "Select a saved .song file";
        return snapshot;
    }
    juce::ZipFile zip(song);
    if (zip.getNumEntries() == 0)
    {
        snapshot.error = "Cannot open .song archive";
        return snapshot;
    }
    auto meta = parse(readEntry(zip, "metainfo.xml", 2 * 1024 * 1024, snapshot.error), "metainfo.xml", snapshot.error);
    if (!snapshot.ok() || meta == nullptr || !meta->hasTagName("MetaInformation"))
    {
        if (snapshot.ok()) snapshot.error = "Invalid metainfo.xml";
        return snapshot;
    }
    juce::String notesMember;
    for (auto* item = meta->getFirstChildElement(); item != nullptr; item = item->getNextElement())
    {
        if (!item->hasTagName("Attribute")) continue;
        const auto key = item->getStringAttribute("id");
        const auto value = item->getStringAttribute("value");
        if (key == "Document:Title") snapshot.documentTitle = value;
        else if (key == "Media:Title") snapshot.mediaTitle = value;
        else if (key == "Media:Artist") snapshot.artist = value;
        else if (key == "Document:Notes") notesMember = value;
    }
    if (notesMember.isNotEmpty())
    {
        if (notesMember != "notes.txt") { snapshot.error = "Unsupported Notes location"; return snapshot; }
        snapshot.notes = readEntry(zip, notesMember, 2 * 1024 * 1024, snapshot.error).trim();
        if (!snapshot.ok()) return snapshot;
    }

    std::map<juce::String, juce::String> trackNotes;
    if (zip.getIndexOfFileName("notepad.xml") >= 0)
    {
        auto notepad = parse(readEntry(zip, "notepad.xml", 2 * 1024 * 1024, snapshot.error),
            "notepad.xml", snapshot.error);
        if (!snapshot.ok() || notepad == nullptr || !notepad->hasTagName("NotepadData"))
        {
            if (snapshot.ok()) snapshot.error = "Invalid notepad.xml";
            return snapshot;
        }
        for (auto* item = notepad->getFirstChildElement(); item != nullptr; item = item->getNextElement())
            if (item->hasTagName("NotepadItem") && item->hasAttribute("id"))
                trackNotes[item->getStringAttribute("id")] = item->getStringAttribute("text");
    }

    auto songXml = parse(readEntry(zip, "Song/song.xml", 64 * 1024 * 1024, snapshot.error), "Song/song.xml", snapshot.error);
    if (!snapshot.ok() || songXml == nullptr || !songXml->hasTagName("Song"))
    {
        if (snapshot.ok()) snapshot.error = "Invalid Song/song.xml";
        return snapshot;
    }
    for (auto* rootAttrs = songXml->getFirstChildElement(); rootAttrs != nullptr; rootAttrs = rootAttrs->getNextElement())
    {
        if (!rootAttrs->hasTagName("Attributes")) continue;
        for (auto* trackList = rootAttrs->getFirstChildElement(); trackList != nullptr; trackList = trackList->getNextElement())
        {
            if (!trackList->hasTagName("List") || trackList->getStringAttribute("x_id") != "Tracks") continue;
            for (auto* track = trackList->getFirstChildElement(); track != nullptr; track = track->getNextElement())
            {
                if (track->hasTagName("ArrangerTrack") || track->hasTagName("MarkerTrack"))
                {
                    const auto arrangerTrack = track->hasTagName("ArrangerTrack");
                    for (auto* item = track->getFirstChildElement(); item != nullptr; item = item->getNextElement())
                    {
                        if (!(arrangerTrack ? item->hasTagName("ArrangerEvent") : item->hasTagName("MarkerEvent"))) continue;
                        TimelineItem entry;
                        entry.type = item->getTagName();
                        entry.name = item->getStringAttribute("name");
                        entry.start = item->getStringAttribute("start", "0");
                        entry.length = item->getStringAttribute("length", "0");
                        entry.timeFormat = item->getStringAttribute("timeFormat", track->getStringAttribute("timeFormat"));
                        entry.color = item->getStringAttribute("color", track->getStringAttribute("color"));
                        (arrangerTrack ? snapshot.sections : snapshot.markers).push_back(std::move(entry));
                    }
                    continue;
                }
                if (!track->hasTagName("MediaTrack")) continue;
                ++snapshot.trackCount;
                SongTrack songTrack;
                songTrack.id = track->getStringAttribute("trackID");
                songTrack.name = track->getStringAttribute("name");
                songTrack.mediaType = track->getStringAttribute("mediaType");
                songTrack.color = track->getStringAttribute("color");
                if (auto note = trackNotes.find(songTrack.id); note != trackNotes.end())
                    songTrack.notes = note->second;
                for (auto* events = track->getFirstChildElement(); events != nullptr; events = events->getNextElement())
                {
                    if (!events->hasTagName("List") || events->getStringAttribute("x_id") != "Events") continue;
                    for (auto* item = events->getFirstChildElement(); item != nullptr; item = item->getNextElement())
                    {
                        if (!item->hasTagName("AudioEvent") && !item->hasTagName("MusicPart")) continue;
                        SongEvent event;
                        event.track = track->getStringAttribute("name");
                        event.trackId = track->getStringAttribute("trackID");
                        event.mediaType = track->getStringAttribute("mediaType");
                        event.type = item->getTagName();
                        event.name = item->getStringAttribute("name");
                        event.start = item->getStringAttribute("start", "0");
                        event.length = item->getStringAttribute("length", "0");
                        event.timeFormat = item->getStringAttribute("timeFormat");
                        songTrack.events.push_back(event);
                        snapshot.events.push_back(std::move(event));
                    }
                }
                snapshot.tracks.push_back(std::move(songTrack));
            }
        }
    }
    return snapshot;
}

std::vector<size_t> matchingSectionIndices(const SongSnapshot& snapshot, const SongEvent& event)
{
    std::vector<size_t> matches;
    const double clipStart = event.start.getDoubleValue();
    const double clipEnd = clipStart + event.length.getDoubleValue();
    if (clipEnd <= clipStart) return matches;
    for (size_t i = 0; i < snapshot.sections.size(); ++i)
    {
        const auto& section = snapshot.sections[i];
        const double start = section.start.getDoubleValue();
        const double end = start + section.length.getDoubleValue();
        if (end > start && clipStart < end && clipEnd > start) matches.push_back(i);
    }
    std::stable_sort(matches.begin(), matches.end(), [&](size_t a, size_t b)
    {
        return snapshot.sections[a].start.getDoubleValue() < snapshot.sections[b].start.getDoubleValue();
    });
    return matches;
}

std::vector<MarkerNote> matchingMarkerNotes(const SongSnapshot& snapshot, const SongEvent& event)
{
    std::vector<MarkerNote> notes;
    const double clipStart = event.start.getDoubleValue();
    const double clipEnd = clipStart + event.length.getDoubleValue();
    if (clipEnd <= clipStart) return notes;
    std::vector<size_t> order(snapshot.markers.size());
    std::iota(order.begin(), order.end(), size_t{0});
    std::stable_sort(order.begin(), order.end(), [&](size_t a, size_t b)
    {
        return snapshot.markers[a].start.getDoubleValue() < snapshot.markers[b].start.getDoubleValue();
    });
    for (const auto index : order)
    {
        const auto& marker = snapshot.markers[index];
        const double position = marker.start.getDoubleValue();
        if (position < clipStart || position >= clipEnd) continue;
        if (const auto note = parseMarkerNote(marker.name)) notes.push_back(*note);
    }
    return notes;
}
}

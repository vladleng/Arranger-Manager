#include "SongSnapshot.h"

namespace arranger
{
namespace
{
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
    stream->readIntoMemoryBlock(data, static_cast<ssize_t>(limit + 1));
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
                if (!track->hasTagName("MediaTrack")) continue;
                ++snapshot.trackCount;
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
                        snapshot.events.push_back(std::move(event));
                    }
                }
            }
        }
    }
    return snapshot;
}
}

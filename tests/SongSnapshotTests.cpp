#include "SongSnapshot.h"
#include <iostream>
#include <string>

namespace
{
void add(juce::ZipFile::Builder& zip, const juce::String& path, const std::string& contents)
{
    zip.addEntry(std::make_unique<juce::MemoryInputStream>(contents.data(), contents.size(), true),
        6, path, juce::Time::getCurrentTime());
}

bool check(bool condition, const char* message)
{
    if (!condition) std::cerr << message << '\n';
    return condition;
}
}

int main()
{
    const auto file = juce::File::getSpecialLocation(juce::File::tempDirectory)
        .getNonexistentChildFile("arranger-song-snapshot", ".song", false);
    juce::ZipFile::Builder zip;
    add(zip, "metainfo.xml", std::string("\xEF\xBB\xBF") +
        "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
        "<MetaInformation><Attribute id=\"Document:Title\" value=\"Test Song\"/>"
        "<Attribute id=\"Media:Artist\" value=\"Artist\"/>"
        "<Attribute id=\"Document:Notes\" value=\"notes.txt\"/></MetaInformation>");
    add(zip, "notes.txt", "Project notes");
    add(zip, "notepad.xml", "<NotepadData>"
        "<NotepadItem id=\"track-1\" title=\"Drums\" text=\"Record five parts\"/>"
        "<NotepadItem id=\"track-2\" title=\"Keys\" text=\"\"/>"
        "</NotepadData>");
    add(zip, "Song/song.xml", std::string("\xEF\xBB\xBF") +
        "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
        "<Song><Attributes x:id=\"Root\"><List x:id=\"Tracks\">"
        "<MediaTrack name=\"Drums\" trackID=\"track-1\" mediaType=\"Audio\" color=\"FF34A9F2\">"
        "<List x:id=\"Events\"><AudioEvent name=\"WIP | P2\" start=\"32\" length=\"8\"/>"
        "</List></MediaTrack>"
        "<MediaTrack name=\"Keys\" trackID=\"track-2\" mediaType=\"Music\">"
        "<List x:id=\"Events\"><MusicPart name=\"DONE | P1\" start=\"40\" length=\"4\"/>"
        "</List></MediaTrack>"
        "<ArrangerTrack color=\"FF8A6B32\"><ArrangerEvent name=\"Intro\" start=\"32\" length=\"8\" color=\"FFFFAD2A\"/>"
        "<ArrangerEvent name=\"Verse\" start=\"40\" length=\"8\"/></ArrangerTrack>"
        "<MarkerTrack><MarkerEvent name=\"Start\"/><MarkerEvent name=\"End\" start=\"48\"/>"
        "</MarkerTrack></List></Attributes></Song>");

    {
        juce::FileOutputStream output(file);
        if (!check(output.openedOk() && zip.writeToStream(output, nullptr), "Cannot create fixture")) return 1;
        output.flush();
    }
    const auto snapshot = arranger::readSongSnapshot(file);
    const auto passed = check(snapshot.ok(), snapshot.error.toRawUTF8())
        && check(snapshot.documentTitle == "Test Song", "Title mismatch")
        && check(snapshot.artist == "Artist" && snapshot.notes == "Project notes", "Metadata mismatch")
        && check(snapshot.trackCount == 2 && snapshot.events.size() == 2, "Event count mismatch")
        && check(snapshot.tracks.size() == 2 && snapshot.tracks[0].events.size() == 1, "Track grouping mismatch")
        && check(snapshot.tracks[0].notes == "Record five parts" && snapshot.tracks[1].notes.isEmpty(), "Track notes mismatch")
        && check(snapshot.tracks[0].color == "FF34A9F2" && snapshot.tracks[1].color.isEmpty(), "Track colour mismatch")
        && check(snapshot.sections.size() == 2 && snapshot.sections[0].name == "Intro"
            && snapshot.sections[0].color == "FFFFAD2A" && snapshot.sections[1].color == "FF8A6B32", "Arrangement mismatch")
        && check(snapshot.markers.size() == 2 && snapshot.markers[0].start == "0"
            && snapshot.markers[1].name == "End", "Marker mismatch")
        && check(snapshot.events[0].name == "WIP | P2" && snapshot.events[0].track == "Drums", "Audio event mismatch")
        && check(snapshot.events[1].name == "DONE | P1" && snapshot.events[1].type == "MusicPart", "MIDI event mismatch")
        && check(arranger::matchingSectionIndices(snapshot, snapshot.events[0]) == std::vector<size_t>{0}, "Intro mapping mismatch")
        && check(arranger::matchingSectionIndices(snapshot, snapshot.events[1]) == std::vector<size_t>{1}, "Verse mapping mismatch")
        && check(arranger::matchingSectionIndices(snapshot,
            arranger::SongEvent{.start = "39", .length = "2"}) == std::vector<size_t>({0, 1}), "Boundary overlap mismatch");
    file.deleteFile();
    return passed ? 0 : 1;
}

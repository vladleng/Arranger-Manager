#include "SongSnapshot.h"
#include "StudioProColour.h"
#include "SongProgress.h"
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
        "<NotepadItem id=\"track-1\" title=\"Drums\" text=\"WIP | Record five parts\"/>"
        "<NotepadItem id=\"track-2\" title=\"Keys\" text=\"\"/>"
        "</NotepadData>");
    add(zip, "Song/song.xml", std::string("\xEF\xBB\xBF") +
        "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
        "<Song><Attributes x:id=\"Root\"><List x:id=\"Tracks\">"
        "<MediaTrack name=\"WIP | Drums\" trackID=\"track-1\" mediaType=\"Audio\" color=\"FF34A9F2\">"
        "<List x:id=\"Events\"><AudioEvent name=\"WIP | Drums | Record five parts\" start=\"32\" length=\"8\"/>"
        "<AudioEvent name=\"DONE | Before Start\" start=\"20\" length=\"4\"/>"
        "<AudioEvent name=\"DONE | Scratch Pad\" start=\"50\" length=\"4\"/>"
        "<AudioEvent name=\"DONE | Beyond End\" start=\"46\" length=\"4\"/>"
        "</List></MediaTrack>"
        "<MediaTrack name=\"Keys\" trackID=\"track-2\" mediaType=\"Music\">"
        "<List x:id=\"Events\"><MusicPart name=\"DONE | P1\" start=\"40\" length=\"8\"/>"
        "<MusicPart name=\"DONE | At End\" start=\"48\" length=\"1\"/>"
        "</List></MediaTrack>"
        "<ArrangerTrack color=\"FF8A6B32\"><ArrangerEvent name=\"Intro\" start=\"32\" length=\"8\" color=\"FFFFAD2A\"/>"
        "<ArrangerEvent name=\"Verse\" start=\"40\" length=\"8\"/></ArrangerTrack>"
        "<MarkerTrack><MarkerEvent name=\"Start\" start=\"32\"/><MarkerEvent name=\"mk1 | Record drums\" start=\"32\"/>"
        "<MarkerEvent name=\"mk2 Check timing\" start=\"34\"/>"
        "<MarkerEvent name=\"Ordinary marker\" start=\"35\"/>"
        "<MarkerEvent name=\"mk3: Next clip\" start=\"40\"/><MarkerEvent name=\"End\" start=\"48\"/>"
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
        && check(snapshot.tracks[0].notes == "WIP | Record five parts" && snapshot.tracks[1].notes.isEmpty(), "Track notes mismatch")
        && check(arranger::parseTrackName(snapshot.tracks[0].name.toStdString()).status == arranger::Status::wip
            && arranger::parseTrackName(snapshot.tracks[0].name.toStdString()).name == "Drums",
            "Track status did not come from its name")
        && check(snapshot.tracks[0].color == "FF34A9F2" && snapshot.tracks[1].color.isEmpty(), "Track colour mismatch")
        && check(snapshot.sections.size() == 2 && snapshot.sections[0].name == "Intro"
            && snapshot.sections[0].color == "FFFFAD2A" && snapshot.sections[1].color == "FF8A6B32", "Arrangement mismatch")
        && check(arranger::studioProColour(snapshot.sections[0].color.toStdString()) == 0xff2aadffu,
            "Studio Pro section ABGR conversion mismatch")
        && check(arranger::studioProColour(snapshot.sections[1].color.toStdString()) == 0xff326b8au,
            "Studio Pro inherited section ABGR conversion mismatch")
        && check(arranger::studioProColour("FFFF2A94") == 0xff942affu,
            "Studio Pro track ABGR conversion mismatch")
        && check(!arranger::studioProColour("FFGG2A94").has_value(), "Invalid color accepted")
        && check(snapshot.markers.size() == 6 && snapshot.markers[0].start == "0"
            && snapshot.markers[5].name == "End", "Marker mismatch")
        && check(snapshot.events[0].name == "WIP | Drums | Record five parts" && snapshot.events[0].track == "WIP | Drums", "Audio event mismatch")
        && check(snapshot.events[1].name == "DONE | P1" && snapshot.events[1].type == "MusicPart", "MIDI event mismatch")
        && check(arranger::matchingSectionIndices(snapshot, snapshot.events[0]) == std::vector<size_t>{0}, "Intro mapping mismatch")
        && check(arranger::matchingSectionIndices(snapshot, snapshot.events[1]) == std::vector<size_t>{1}, "Verse mapping mismatch")
        && check(arranger::parse(snapshot.events[0].name.toStdString()).title == "Drums"
            && arranger::parse(snapshot.events[0].name.toStdString()).note == "Record five parts",
            "Clip title and note mismatch")
        && check(snapshot.tracks[0].events.size() == 1 && snapshot.events.size() == 2
            && snapshot.clipRangeWarning.isEmpty(), "Events outside Start/End entered snapshot")
        && check(arranger::matchingSectionIndices(snapshot,
            arranger::SongEvent{.start = "39", .length = "2"}) == std::vector<size_t>({0, 1}), "Boundary overlap mismatch")
        && check(arranger::clipProgress(snapshot.events) == std::pair<int, int>{1, 2}, "Tagged clip progress mismatch")
        && check(arranger::clipProgress({snapshot.events[0], snapshot.events[1],
            arranger::SongEvent{.name = "Plain take"}}) == std::pair<int, int>{1, 2},
            "Unmarked clip entered progress denominator")
        && check(arranger::managedTrackCount(snapshot) == 1 && arranger::songProgress(snapshot)
            == std::pair<int, int>{0, 1}, "Song progress must count marked track statuses, not clips");
    auto markedSong = snapshot;
    markedSong.tracks[1].name = "DONE | Keys";
    auto completedClipOnOpenTrack = snapshot;
    completedClipOnOpenTrack.tracks[0].events[0].name = "DONE | Take ready";
    const auto additional = check(arranger::managedTrackCount(markedSong) == 2
        && arranger::songProgress(markedSong) == std::pair<int, int>{1, 2},
        "DONE track not included in song progress")
        && check(arranger::songProgress(completedClipOnOpenTrack) == std::pair<int, int>{0, 1},
            "DONE clip must not complete a WIP track in song progress")
        && check(arranger::clipProgress(markedSong.tracks[0].events) == std::pair<int, int>{0, 1}
            && arranger::clipProgress(markedSong.tracks[1].events) == std::pair<int, int>{1, 1},
            "Track progress must still count tagged clips");
    file.deleteFile();
    juce::ZipFile::Builder invalidZip;
    add(invalidZip, "metainfo.xml", "<MetaInformation/>");
    add(invalidZip, "Song/song.xml", "<Song><Attributes x:id=\"Root\"><List x:id=\"Tracks\">"
        "<MediaTrack name=\"WIP | Drums\"><List x:id=\"Events\">"
        "<AudioEvent name=\"DONE | Take\" start=\"1\" length=\"1\"/>"
        "</List></MediaTrack><MarkerTrack><MarkerEvent name=\"Start\" start=\"0\"/>"
        "</MarkerTrack></List></Attributes></Song>");
    {
        juce::FileOutputStream output(file);
        if (!check(output.openedOk() && invalidZip.writeToStream(output, nullptr), "Cannot create invalid-range fixture")) return 1;
        output.flush();
    }
    const auto invalid = arranger::readSongSnapshot(file);
    const auto missingRange = check(invalid.ok() && invalid.events.empty()
        && invalid.tracks.size() == 1 && invalid.tracks[0].events.empty()
        && invalid.clipRangeWarning.isNotEmpty(), "Missing End must hide clips with warning");
    file.deleteFile();
    return passed && additional && missingRange ? 0 : 1;
}

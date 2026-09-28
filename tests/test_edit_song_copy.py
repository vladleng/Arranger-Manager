import hashlib
from pathlib import Path
import sys
import tempfile
import unittest
from unittest.mock import patch
import warnings
from zipfile import ZipFile, ZipInfo, ZIP_DEFLATED

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "tools"))
import edit_song_copy
from edit_song_copy import EventSelector, write_song_copy


XML = b'''<?xml version="1.0" encoding="UTF-8"?>
<Song><Attributes x:id="Root"><List x:id="Tracks">
<MediaTrack name="WIP | Drums" trackID="track-1"><List x:id="Events">
<AudioEvent name='WIP | Original' clipID="shared" start="32" length="8" timeFormat="musical"/>
<AudioEvent name='WIP | Original' clipID="shared" start="40" length="8" timeFormat="musical"/>
<AudioEvent name='WIP | Original' clipID="shared" start="40" length="8" timeFormat="seconds"/>
</List></MediaTrack>
<MediaTrack name="WIP | Keys" trackID="track-2"><List x:id="Events">
<MusicPart name="TODO | Chords" clipID="shared" start="32" length="8"/>
</List></MediaTrack>
</List></Attributes></Song>'''


def fixture(path: Path, xml: bytes = XML):
    with ZipFile(path, "w") as archive:
        archive.comment = b"studio project"
        archive.writestr("metainfo.xml", b"<MetaInformation/>")
        info = ZipInfo("Song/song.xml", (2026, 9, 28, 14, 0, 0))
        info.compress_type = ZIP_DEFLATED
        info.comment = b"song entry"
        archive.writestr(info, xml)
        archive.writestr("Media/audio.wav", b"RIFF" + bytes(range(256)) * 40)
        archive.writestr("notepad.xml", b"<NotepadData/>")


class CopyEditTests(unittest.TestCase):
    def setUp(self):
        self.folder = tempfile.TemporaryDirectory()
        self.root = Path(self.folder.name)
        self.source = self.root / "original.song"
        self.output = self.root / "edited.song"
        fixture(self.source)
        self.before = hashlib.sha256(self.source.read_bytes()).digest()

    def tearDown(self):
        self.folder.cleanup()

    def test_audio_then_midi_in_separate_copies(self):
        fixture(self.source, b"\xef\xbb\xbf" + XML)  # Studio Pro may include a UTF-8 BOM.
        self.before = hashlib.sha256(self.source.read_bytes()).digest()
        target = EventSelector("track-1", "AudioEvent", "WIP | Original", "40", "8", "musical")
        write_song_copy(self.source, self.output, target, "DONE | Drums | Attack & release <check>")
        self.assertEqual(hashlib.sha256(self.source.read_bytes()).digest(), self.before)
        with ZipFile(self.source) as old, ZipFile(self.output) as new:
            changed = new.read("Song/song.xml")
            self.assertTrue(changed.startswith(b"\xef\xbb\xbf"))
            self.assertIn(b"name='DONE | Drums | Attack &amp; release &lt;check&gt;'", changed)
            self.assertEqual(changed.count(b"WIP | Original"), 2)
            self.assertEqual(old.read("Media/audio.wav"), new.read("Media/audio.wav"))
            self.assertEqual(old.read("notepad.xml"), new.read("notepad.xml"))
            self.assertEqual(old.comment, new.comment)
            self.assertEqual(old.getinfo("Song/song.xml").comment,
                             new.getinfo("Song/song.xml").comment)

        midi = EventSelector("track-2", "MusicPart", "TODO | Chords", "32", "8")
        second = self.root / "midi.song"
        write_song_copy(self.output, second, midi, 'DONE | Piano | Check "voicing"')
        with ZipFile(second) as archive:
            self.assertIn(b'name="DONE | Piano | Check &quot;voicing&quot;"',
                          archive.read("Song/song.xml"))

    def test_ambiguous_selection_does_not_create_copy(self):
        duplicated = XML.replace(b'<AudioEvent name=\'WIP | Original\' clipID="shared" start="40" length="8" timeFormat="musical"/>',
                                 b'<AudioEvent name=\'WIP | Original\' clipID="shared" start="40" length="8" timeFormat="musical"/>' * 2)
        fixture(self.source, duplicated)
        target = EventSelector("track-1", "AudioEvent", "WIP | Original", "40", "8", "musical", "shared")
        with self.assertRaisesRegex(ValueError, "found 2"):
            write_song_copy(self.source, self.output, target, "DONE | Drums")
        self.assertFalse(self.output.exists())

    def test_missing_target_or_existing_output_does_not_write(self):
        target = EventSelector("track-1", "AudioEvent", "WIP | Original", "99", "8", "musical")
        with self.assertRaisesRegex(ValueError, "found 0"):
            write_song_copy(self.source, self.output, target, "DONE | Drums")
        self.assertFalse(self.output.exists())
        self.output.write_bytes(b"existing")
        with self.assertRaisesRegex(ValueError, "new path"):
            write_song_copy(self.source, self.output, target, "DONE | Drums")
        self.assertEqual(self.output.read_bytes(), b"existing")

    def test_bad_xml_or_duplicate_zip_paths(self):
        target = EventSelector("track-1", "AudioEvent", "WIP | Original", "32", "8", "musical")
        fixture(self.source, XML.replace(b"<Song>", b"<Other>").replace(b"</Song>", b"</Other>"))
        with self.assertRaisesRegex(ValueError, "root"):
            write_song_copy(self.source, self.output, target, "DONE | Drums")
        self.assertFalse(self.output.exists())
        with warnings.catch_warnings():
            warnings.simplefilter("ignore", UserWarning)
            with ZipFile(self.source, "a") as archive:
                archive.writestr("Song/song.xml", XML)
        with self.assertRaisesRegex(ValueError, "Duplicate ZIP paths"):
            write_song_copy(self.source, self.output, target, "DONE | Drums")
        self.assertFalse(self.output.exists())

    def test_source_change_during_copy_discards_output(self):
        target = EventSelector("track-1", "AudioEvent", "WIP | Original", "32", "8", "musical")
        verify = edit_song_copy._verify

        def change_source(original, output, expected):
            verify(original, output, expected)
            with self.source.open("ab") as source:
                source.write(b"changed during copy")

        with patch.object(edit_song_copy, "_verify", side_effect=change_source):
            with self.assertRaisesRegex(ValueError, "Source changed"):
                write_song_copy(self.source, self.output, target, "DONE | Drums")
        self.assertFalse(self.output.exists())


if __name__ == "__main__":
    unittest.main()

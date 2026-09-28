#include "ArrangementTag.h"
#include <iostream>

namespace
{
bool check(bool ok, const char* message)
{
    if (!ok) std::cerr << message << '\n';
    return ok;
}
}

int main()
{
    using namespace arranger;
    const auto wip = parse(" WIP | p1 | переход в припев ");
    bool passed = check(wip.valid && wip.status == Status::wip && wip.priority == Priority::p1
        && wip.note == "переход в припев", "WIP tag");
    passed &= check(parse("DONE | P2").valid, "DONE tag");
    passed &= check(parse("POOL | P2").valid && parse("POOL | P2").status == Status::pool, "POOL tag");
    passed &= check(parse("WAIT | P3").status == Status::wait, "WAIT tag");
    passed &= check(parse("REVIEW | P3").status == Status::wait
        && std::string(label(parse("REVIEW | P3").status)) == "WAIT", "legacy REVIEW alias");
    passed &= check(parse("TODO | P4").priority == Priority::p4, "P4 tag");
    passed &= check(parse("BRASS:DONE").status == Status::done, "legacy group tag");
    passed &= check(parse("[Intro]").status == Status::unmarked, "ordinary clip");
    passed &= check(!parse("WIP | P9 | bad priority").valid, "invalid priority");
    passed &= check(!parse("WIP | P1junk").valid, "malformed priority");
    passed &= check(!parse("BLOCKED | P0 | waiting").valid, "retired P0");
    passed &= check(displayClipName("TODO | P2", "Audio") == "Audio", "status and priority hidden from title");
    passed &= check(displayClipName("REVIEW | P3", "MIDI") == "MIDI", "legacy status hidden from title");
    passed &= check(displayClipName("WIP", "Audio") == "Audio", "status-only title");
    passed &= check(displayClipName("WIP | P2 | Сделать 5 партий", "Audio") == "Сделать 5 партий", "tag note title");
    passed &= check(displayClipName("[Verse 2]", "Audio") == "[Verse 2]", "ordinary title preserved");
    passed &= check(parse("WIP | Сделать 5 партий").valid && parse("WIP | Сделать 5 партий").priority == Priority::none,
        "status and note without priority");
    passed &= check(displayClipName("WIP | Сделать 5 партий", "Audio") == "Сделать 5 партий", "new clip title");
    passed &= check(displayClipName("WAIT | микс | переход", "Audio") == "микс | переход", "note with separator");
    passed &= check(parseTrackName("WIP").status == Status::wip && parseTrackName("WIP").name.empty(),
        "status-only track name");
    passed &= check(parseTrackName("WAIT | Гитара | дубль").status == Status::wait
        && parseTrackName("WAIT | Гитара | дубль").name == "Гитара | дубль", "track name after status");
    passed &= check(parseTrackName("Сделать WIP партию").status == Status::unmarked
        && parseTrackName("Сделать WIP партию").name == "Сделать WIP партию",
        "prose not mistaken for status");
    passed &= check(parseTrackName("REVIEW | Старое имя").status == Status::wait, "legacy status alias");
    passed &= check(parseTrackName("WIP Drums").status == Status::unmarked, "require status delimiter");
    return passed ? 0 : 1;
}

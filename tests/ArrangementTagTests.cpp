#include "ArrangementTag.h"
#include <cassert>

int main()
{
    using namespace arranger;
    auto tag = parse(" WIP | p1 | переход в припев ");
    assert(tag.valid && tag.status == Status::wip && tag.priority == Priority::p1);
    assert(tag.note == "переход в припев");
    assert(parse("DONE | P2").valid);
    assert(parse("BRASS:DONE").status == Status::done);
    assert(parse("[Intro]").status == Status::unmarked);
    assert(! parse("WIP | P9 | bad priority").valid);
    assert(! parse("WIP | P1junk").valid);
    assert(parse("BLOCKED | P0 | waiting").priority == Priority::p0);
}

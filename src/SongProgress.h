#pragma once

#include "ArrangementTag.h"
#include "SongSnapshot.h"
#include <utility>

namespace arranger
{
// Untagged audio/MIDI events are ordinary DAW clips, not tasks in progress.
inline std::pair<int, int> clipProgress(const std::vector<SongEvent>& events)
{
    int done = 0, tagged = 0;
    for (const auto& event : events)
    {
        const auto tag = parse(event.name.toStdString());
        if (!tag.valid) continue;
        ++tagged;
        if (tag.status == Status::done) ++done;
    }
    return {done, tagged};
}
}

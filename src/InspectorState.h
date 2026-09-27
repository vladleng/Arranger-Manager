#pragma once

#include <JuceHeader.h>
#include "SharedArrangementMap.h"
#include <mutex>
#include <unordered_map>
#include <vector>

struct InspectedRegion
{
    int sequenceIndex = 0;
    int regionIndex = 0;
    juce::String sequenceName, regionName, effectiveName;
    juce::String sequenceColor, regionColor;
    double startSeconds = 0.0, durationSeconds = 0.0;
};

struct InspectorSnapshot
{
    juce::String documentName;
    int musicalContexts = 0;
    int sequenceCount = 0;
    int regionCount = 0;
    unsigned long long revision = 0;
    juce::String lastChange = "initial scan";
    std::vector<InspectedRegion> regions;
    std::vector<juce::String> emptySequences;
};

class InspectorState final
{
public:
    static InspectorState& instance() { static InspectorState state; return state; }

    void publish(const void* controller, InspectorSnapshot snapshot)
    {
        const std::scoped_lock lock(mutex);
        snapshots[controller] = std::move(snapshot);
        publishShared();
    }

    void remove(const void* controller)
    {
        const std::scoped_lock lock(mutex);
        snapshots.erase(controller);
        publishShared();
    }

    std::vector<InspectorSnapshot> read() const
    {
        const std::scoped_lock lock(mutex);
        std::vector<InspectorSnapshot> result;
        for (const auto& entry : snapshots)
            result.push_back(entry.second);
        return result;
    }

private:
    void publishShared()
    {
        arranger::SharedMap map;
        map.revision = ++revision;
        map.documents = static_cast<std::uint32_t>(snapshots.size());
        for (const auto& [source, snapshot] : snapshots)
        {
            juce::ignoreUnused(source);
            for (const auto& region : snapshot.regions)
            {
                if (map.count >= arranger::maxSharedRegions) break;
                auto& row = map.regions[map.count++];
                region.sequenceName.copyToUTF8(row.track, sizeof(row.track));
                region.effectiveName.copyToUTF8(row.name, sizeof(row.name));
                row.startSeconds = region.startSeconds;
                row.durationSeconds = region.durationSeconds;
            }
        }
        arranger::SharedArrangementMap::instance().publish(map);
    }

    std::uint64_t revision = 0;
    mutable std::mutex mutex;
    std::unordered_map<const void*, InspectorSnapshot> snapshots;
};

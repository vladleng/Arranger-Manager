#pragma once

#include <JuceHeader.h>
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
    }

    void remove(const void* controller)
    {
        const std::scoped_lock lock(mutex);
        snapshots.erase(controller);
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
    mutable std::mutex mutex;
    std::unordered_map<const void*, InspectorSnapshot> snapshots;
};

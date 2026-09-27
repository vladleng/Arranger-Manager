#include "InspectorDocumentController.h"
#include "InspectorState.h"

#if JucePlugin_Enable_ARA
#include <ARA_Library/PlugIn/ARAPlug.h>

namespace
{
template <typename T>
juce::String nameOf(const T& optional)
{
    const char* value = optional;
    return value != nullptr ? juce::String::fromUTF8(value) : juce::String("<not provided>");
}

template <typename T>
juce::String colorOf(const T& optional)
{
    const ARA::ARAColor* color = optional;
    if (color == nullptr) return "<not provided>";
    return juce::String(color->r, 3) + "," + juce::String(color->g, 3) + "," + juce::String(color->b, 3);
}
}

InspectorDocumentController::InspectorDocumentController(const ARA::PlugIn::PlugInEntry* entry,
    const ARA::ARADocumentControllerHostInstance* host)
    : juce::ARADocumentControllerSpecialisation(entry, host)
{
    refresh("controller created");
}

InspectorDocumentController::~InspectorDocumentController() { InspectorState::instance().remove(this); }

bool InspectorDocumentController::doRestoreObjectsFromStream(juce::ARAInputStream& input,
    const juce::ARARestoreObjectsFilter* filter)
{
    juce::ignoreUnused(input, filter);
    return true; // No owned model data in this read-only probe.
}

bool InspectorDocumentController::doStoreObjectsToStream(juce::ARAOutputStream& output,
    const juce::ARAStoreObjectsFilter* filter)
{
    juce::ignoreUnused(output, filter);
    return true;
}

void InspectorDocumentController::refresh(const char* reason, const juce::ARARegionSequence* removedSequence,
    const juce::ARAPlaybackRegion* removedRegion)
{
    InspectorSnapshot snapshot;
    snapshot.revision = ++revision;
    snapshot.lastChange = reason;
    if (auto* document = getDocument())
    {
        snapshot.documentName = nameOf(document->getName());
        snapshot.musicalContexts = static_cast<int>(document->getMusicalContexts().size());
        const auto& sequences = document->getRegionSequences();
        for (int si = 0; si < static_cast<int>(sequences.size()); ++si)
        {
            const auto* sequence = sequences[static_cast<size_t>(si)];
            if (sequence == removedSequence) continue;
            ++snapshot.sequenceCount;
            const auto& regions = sequence->getPlaybackRegions();
            if (regions.empty() || (regions.size() == 1 && regions.front() == removedRegion))
                snapshot.emptySequences.push_back(juce::String(si + 1) + ": " + nameOf(sequence->getName()));
            for (int ri = 0; ri < static_cast<int>(regions.size()); ++ri)
            {
                const auto* region = regions[static_cast<size_t>(ri)];
                if (region == removedRegion) continue;
                InspectedRegion row;
                row.sequenceIndex = si + 1;
                row.regionIndex = ri + 1;
                row.sequenceName = nameOf(sequence->getName());
                row.sequenceColor = colorOf(sequence->getColor());
                row.regionName = nameOf(region->getName());
                row.effectiveName = nameOf(region->getEffectiveName());
                row.regionColor = colorOf(region->getColor());
                row.startSeconds = region->getStartInPlaybackTime();
                row.durationSeconds = region->getDurationInPlaybackTime();
                snapshot.regions.push_back(std::move(row));
            }
        }
        snapshot.regionCount = static_cast<int>(snapshot.regions.size());
    }
    InspectorState::instance().publish(this, std::move(snapshot));
}

void InspectorDocumentController::didAddRegionSequenceToDocument(juce::ARADocument*, juce::ARARegionSequence*) { refresh("sequence added"); }
void InspectorDocumentController::willRemoveRegionSequenceFromDocument(juce::ARADocument*, juce::ARARegionSequence* sequence) { refresh("sequence removed", sequence); }
void InspectorDocumentController::didReorderRegionSequencesInDocument(juce::ARADocument*) { refresh("sequences reordered"); }
void InspectorDocumentController::didUpdateRegionSequenceProperties(juce::ARARegionSequence*) { refresh("sequence properties updated"); }
void InspectorDocumentController::didAddPlaybackRegionToRegionSequence(juce::ARARegionSequence*, juce::ARAPlaybackRegion*) { refresh("region added"); }
void InspectorDocumentController::willRemovePlaybackRegionFromRegionSequence(juce::ARARegionSequence*, juce::ARAPlaybackRegion* region) { refresh("region removed", nullptr, region); }
void InspectorDocumentController::didUpdatePlaybackRegionProperties(juce::ARAPlaybackRegion*) { refresh("region properties updated"); }
void InspectorDocumentController::didAddMusicalContextToDocument(juce::ARADocument*, juce::ARAMusicalContext*) { refresh("musical context added"); }
void InspectorDocumentController::willRemoveMusicalContextFromDocument(juce::ARADocument*, juce::ARAMusicalContext*) { refresh("musical context removing"); }
#endif

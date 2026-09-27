#pragma once
#include <JuceHeader.h>

#if JucePlugin_Enable_ARA
class InspectorDocumentController final : public juce::ARADocumentControllerSpecialisation
{
public:
    InspectorDocumentController(const ARA::PlugIn::PlugInEntry*, const ARA::ARADocumentControllerHostInstance*);
    ~InspectorDocumentController() override;

protected:
    bool doRestoreObjectsFromStream(juce::ARAInputStream&, const juce::ARARestoreObjectsFilter*) override;
    bool doStoreObjectsToStream(juce::ARAOutputStream&, const juce::ARAStoreObjectsFilter*) override;

    void didAddRegionSequenceToDocument(juce::ARADocument*, juce::ARARegionSequence*) override;
    void willRemoveRegionSequenceFromDocument(juce::ARADocument*, juce::ARARegionSequence*) override;
    void didReorderRegionSequencesInDocument(juce::ARADocument*) override;
    void didUpdateRegionSequenceProperties(juce::ARARegionSequence*) override;
    void didAddPlaybackRegionToRegionSequence(juce::ARARegionSequence*, juce::ARAPlaybackRegion*) override;
    void willRemovePlaybackRegionFromRegionSequence(juce::ARARegionSequence*, juce::ARAPlaybackRegion*) override;
    void didUpdatePlaybackRegionProperties(juce::ARAPlaybackRegion*) override;
    void didAddMusicalContextToDocument(juce::ARADocument*, juce::ARAMusicalContext*) override;
    void willRemoveMusicalContextFromDocument(juce::ARADocument*, juce::ARAMusicalContext*) override;

private:
    void refresh(const char* reason, const juce::ARARegionSequence* removedSequence = nullptr,
                 const juce::ARAPlaybackRegion* removedRegion = nullptr);
    unsigned long long revision = 0;
};
#endif

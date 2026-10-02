#pragma once
#include <JuceHeader.h>
#include "WorkspaceCommands.h"
#include "BlockEditor.h"

class TaskDetails final : public juce::Component, private juce::Timer, private juce::ListBoxModel
{
public:
    using Execute = std::function<juce::Result(const arranger::WorkspaceAction&)>;
    TaskDetails(arranger::WorkspaceModel&, Execute, std::function<void(juce::String)> back,
        std::function<void(juce::String)> archive);
    juce::Result bind(const juce::String&);
    juce::Result flush();
    void resized() override;
private:
    int getNumRows() override;
    void paintListBoxItem(int, juce::Graphics&, int, int, bool) override;
    void listBoxItemDoubleClicked(int, const juce::MouseEvent&) override;
    void selectedRowsChanged(int) override;
    void timerCallback() override;
    void refreshCheckpoints();
    void promptCheckpoint(bool create);
    void archiveCheckpoint();
    void setStatus(const juce::Result&);
    arranger::TaskProperties fields() const;
    void showProperties();
    const arranger::TaskCheckpoint* selectedCheckpoint() const;

    arranger::WorkspaceModel& model;
    Execute execute;
    std::function<void(juce::String)> back, archiveTask;
    juce::String taskId;
    arranger::TaskProperties baseline;
    bool loading = false;
    juce::TextEditor name, notes;
    juce::ComboBox status, priority;
    juce::TextButton save, reloadProperties, owner, archiveButton, addCheckpoint, editCheckpoint, removeCheckpoint;
    juce::ToggleButton archivedCheckpoints;
    juce::Label noteLabel, cpLabel, feedback;
    juce::ListBox checkpoints { "Checkpoints", this };
    std::vector<juce::String> checkpointIds;
    BlockEditor description;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TaskDetails)
};

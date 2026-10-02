#pragma once
#include <juce_gui_extra/juce_gui_extra.h>
#include "BlockEditorSession.h"

// Reusable UI; document ownership/storage are supplied through callbacks.
class BlockEditor final : public juce::Component, private juce::Timer,
    private juce::ListBoxModel, private juce::KeyListener
{
public:
    using Session = arranger::BlockEditorSession;
    BlockEditor();
    juce::Result bind(const juce::String&, Session::Read, Session::Write);
    juce::Result flush();
    void paint(juce::Graphics&) override;
    void resized() override;
private:
    int getNumRows() override;
    void paintListBoxItem(int, juce::Graphics&, int, int, bool) override;
    void selectedRowsChanged(int) override;
    bool keyPressed(const juce::KeyPress&, juce::Component*) override;
    void timerCallback() override;
    void showBlock();
    void refreshRows(bool showFields);
    void stageFields();
    void addBlock();
    void deleteBlock();
    void moveBlock(int);
    void history(bool redo);
    void updateStatus(const juce::Result&);
    static juce::String tr(const char*);
    Session session;
    juce::String boundId, selectedId;
    int selected = -1;
    bool loading = false;
    juce::ListBox list { "Blocks", this };
    juce::ComboBox newKind, kind;
    juce::TextButton add, remove, up, down, undoButton, redoButton, saveButton, reloadButton, openLink;
    juce::Label nameLabel, typeLabel, textLabel, urlLabel, status, hint;
    juce::TextEditor nameField, textField, urlField;
    juce::ToggleButton checked;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BlockEditor)
};

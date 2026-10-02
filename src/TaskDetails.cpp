#include "TaskDetails.h"
namespace
{
juce::String tr(const char* value) { return juce::String::fromUTF8(value); }
const arranger::Status statuses[] = {arranger::Status::pool, arranger::Status::todo, arranger::Status::wip,
    arranger::Status::draft, arranger::Status::wait, arranger::Status::done, arranger::Status::blocked};
int statusId(arranger::Status value)
{
    for (int i = 0; i < 7; ++i) if (statuses[i] == value) return i + 1;
    return 2;
}
}
TaskDetails::TaskDetails(arranger::WorkspaceModel& data, Execute run,
    std::function<void(juce::String)> goBack, std::function<void(juce::String)> archive)
    : model(data), execute(std::move(run)), back(std::move(goBack)), archiveTask(std::move(archive))
{
    for (auto* field : { &name, &notes })
    {
        addAndMakeVisible(field);
        field->setFont(juce::FontOptions(16.0f));
        field->onTextChange = [this] { if (!loading) startTimer(700); };
    }
    notes.setMultiLine(true); notes.setReturnKeyStartsNewLine(true);
    name.setTextToShowWhenEmpty(tr("Название задачи"), juce::Colours::grey);
    status.addItemList({"POOL", "TODO", "WIP", "DRAFT", "WAIT", "DONE", "BLOCKED"}, 1);
    priority.addItemList({"P1", "P2", "P3", "P4"}, 1);
    for (auto* combo : { &status, &priority })
    {
        addAndMakeVisible(combo); combo->onChange = [this] { if (!loading) startTimer(700); };
    }
    const char* titles[] = {"Сохранить", "Перечитать", "К странице", "Удалить в архив", "+ Чек-поинт", "Изменить", "В архив"};
    int i = 0;
    for (auto* button : { &save, &reloadProperties, &owner, &archiveButton, &addCheckpoint, &editCheckpoint, &removeCheckpoint })
    {
        addAndMakeVisible(button); button->setButtonText(tr(titles[i++]));
    }
    for (auto* label : { &noteLabel, &cpLabel, &feedback })
    {
        addAndMakeVisible(label); label->setColour(juce::Label::textColourId, juce::Colour(0xffb8c5d5));
    }
    noteLabel.setText("Notes", juce::dontSendNotification);
    cpLabel.setText(tr("Чек-поинты"), juce::dontSendNotification);
    addAndMakeVisible(archivedCheckpoints); archivedCheckpoints.setButtonText(tr("Архив чек-поинтов"));
    addAndMakeVisible(checkpoints); checkpoints.setRowHeight(60);
    checkpoints.setColour(juce::ListBox::backgroundColourId, juce::Colour(0xff202832));
    addAndMakeVisible(description);
    save.onClick = [this] { flush(); };
    reloadProperties.onClick = [this]
    {
        stopTimer();
        juce::Component::SafePointer<TaskDetails> safe(this);
        juce::AlertWindow::showOkCancelBox(juce::MessageBoxIconType::QuestionIcon,
            tr("Перечитать свойства"), tr("Отбросить несохранённое название, статус, приоритет и Notes? Описание перечитывается отдельно."),
            tr("Перечитать"), tr("Отмена"), this, juce::ModalCallbackFunction::create([safe](int choice)
            {
                if (safe == nullptr) return;
                if (choice != 1) { safe->startTimer(700); return; }
                if (const auto* task = safe->model.findTask(safe->taskId))
                {
                    safe->baseline = task->properties; safe->showProperties(); safe->setStatus(juce::Result::ok());
                }
            }));
    };
    owner.onClick = [this]
    {
        if (flush().failed()) return;
        if (const auto* task = model.findTask(taskId)) back(task->ownerPageId);
    };
    archiveButton.onClick = [this] { if (flush().wasOk()) archiveTask(taskId); };
    addCheckpoint.onClick = [this] { promptCheckpoint(true); };
    editCheckpoint.onClick = [this] { promptCheckpoint(false); };
    removeCheckpoint.onClick = [this] { archiveCheckpoint(); };
    archivedCheckpoints.onClick = [this] { refreshCheckpoints(); };
}
arranger::TaskProperties TaskDetails::fields() const
{
    return { name.getText(), notes.getText(), statuses[juce::jlimit(1, 7, status.getSelectedId()) - 1],
        juce::jlimit(1, 4, priority.getSelectedId()) };
}
void TaskDetails::showProperties()
{
    loading = true;
    name.setText(baseline.name, false); notes.setText(baseline.notes, false);
    status.setSelectedId(statusId(baseline.status), juce::dontSendNotification);
    priority.setSelectedId(baseline.priority, juce::dontSendNotification);
    loading = false;
}
void TaskDetails::setStatus(const juce::Result& result)
{
    feedback.setText(result.failed() ? tr("Не сохранено: ") + result.getErrorMessage() : tr("Свойства сохранены"), juce::dontSendNotification);
}
juce::Result TaskDetails::flush()
{
    stopTimer();
    if (taskId.isNotEmpty())
    {
        auto values = fields(); values.name = values.name.trim();
        if (values != baseline)
        {
            const auto id = taskId;
            const auto result = execute([&](arranger::WorkspaceCommands& cmd) { return cmd.updateTask(id, baseline, values); });
            setStatus(result);
            if (result.failed()) return result;
            baseline = values;
        }
    }
    const auto saved = description.flush(); setStatus(saved); return saved;
}
juce::Result TaskDetails::bind(const juce::String& id)
{
    if (id != taskId)
    {
        const auto saved = flush(); if (saved.failed()) return saved;
        taskId = id;
        if (id.isEmpty()) return description.bind({}, {}, {});
        const auto* task = model.findTask(id);
        if (!task || arranger::WorkspaceQueries::taskArchived(model, id)) return juce::Result::fail("Task is not editable.");
        baseline = task->properties; showProperties(); archivedCheckpoints.setToggleState(false, juce::dontSendNotification);
    }
    else if (id.isNotEmpty())
    {
        const auto* task = model.findTask(id);
        if (!task) return juce::Result::fail("Task was not found.");
        if (fields() == baseline) { baseline = task->properties; showProperties(); }
        else if (task->properties != baseline) return juce::Result::fail("Task properties changed while edits were pending.");
    }
    if (id.isEmpty()) return juce::Result::ok();
    const auto result = description.bind("task:" + id, [this, id]() -> std::optional<arranger::BlockEditorSession::Blocks>
    {
        const auto* task = model.findTask(id);
        if (!task || arranger::WorkspaceQueries::taskArchived(model, id)) return {};
        return task->blocks;
    }, [this, id](const auto& expected, const auto& blocks)
    {
        return execute([&](arranger::WorkspaceCommands& cmd) { return cmd.replaceTaskBlocks(id, expected, blocks); });
    });
    refreshCheckpoints(); return result;
}
void TaskDetails::timerCallback() { flush(); }
void TaskDetails::resized()
{
    auto area = getLocalBounds();
    auto top = area.removeFromTop(34);
    archiveButton.setBounds(top.removeFromRight(138).reduced(2));
    owner.setBounds(top.removeFromRight(104).reduced(2));
    save.setBounds(top.removeFromRight(96).reduced(2));
    reloadProperties.setBounds(top.removeFromRight(108).reduced(2));
    priority.setBounds(top.removeFromRight(60).reduced(2));
    status.setBounds(top.removeFromRight(96).reduced(2));
    name.setBounds(top.reduced(2));
    auto noteRow = area.removeFromTop(56);
    noteLabel.setBounds(noteRow.removeFromLeft(48));
    notes.setBounds(noteRow.reduced(2));
    feedback.setBounds(area.removeFromBottom(26));
    area.removeFromTop(8);
    auto cpArea = area.removeFromLeft(210);
    cpLabel.setBounds(cpArea.removeFromTop(26));
    addCheckpoint.setBounds(cpArea.removeFromTop(30).reduced(2));
    archivedCheckpoints.setBounds(cpArea.removeFromBottom(28));
    auto buttons = cpArea.removeFromBottom(32);
    editCheckpoint.setBounds(buttons.removeFromLeft(96).reduced(2));
    removeCheckpoint.setBounds(buttons.reduced(2));
    checkpoints.setBounds(cpArea);
    area.removeFromLeft(12);
    description.setBounds(area);
}
const arranger::TaskCheckpoint* TaskDetails::selectedCheckpoint() const
{
    const int index = checkpoints.getSelectedRow();
    if (index < 0 || index >= static_cast<int>(checkpointIds.size())) return nullptr;
    if (const auto* task = model.findTask(taskId))
        for (const auto& cp : task->checkpoints) if (cp.id == checkpointIds[static_cast<size_t>(index)]) return &cp;
    return nullptr;
}
void TaskDetails::refreshCheckpoints()
{
    juce::String selectedId;
    if (const auto* cp = selectedCheckpoint()) selectedId = cp->id;
    checkpointIds.clear();
    if (const auto* task = model.findTask(taskId))
    {
        const auto progress = arranger::WorkspaceQueries::taskProgress(*task);
        cpLabel.setText(tr("Чек-поинты: ") + juce::String(progress.first) + "/" + juce::String(progress.second), juce::dontSendNotification);
        for (const auto& cp : task->checkpoints)
            if (cp.archivedAt.isNotEmpty() == archivedCheckpoints.getToggleState()) checkpointIds.push_back(cp.id);
    }
    checkpoints.updateContent(); checkpoints.deselectAllRows();
    for (int i = 0; i < static_cast<int>(checkpointIds.size()); ++i) if (checkpointIds[static_cast<size_t>(i)] == selectedId) checkpoints.selectRow(i);
    removeCheckpoint.setButtonText(archivedCheckpoints.getToggleState() ? tr("Вернуть") : tr("В архив"));
    editCheckpoint.setEnabled(selectedCheckpoint() != nullptr && !archivedCheckpoints.getToggleState());
    removeCheckpoint.setEnabled(selectedCheckpoint() != nullptr); checkpoints.repaint();
}
int TaskDetails::getNumRows() { return static_cast<int>(checkpointIds.size()); }
void TaskDetails::paintListBoxItem(int index, juce::Graphics& g, int width, int height, bool selected)
{
    if (index < 0 || index >= getNumRows()) return;
    const auto* task = model.findTask(taskId); if (!task) return;
    for (const auto& cp : task->checkpoints) if (cp.id == checkpointIds[static_cast<size_t>(index)])
    {
        if (selected) { g.setColour(juce::Colour(0xff33475f)); g.fillRoundedRectangle(2, 1, static_cast<float>(width - 4), static_cast<float>(height - 2), 5); }
        g.setColour(juce::Colour(0xffe6edf5)); g.setFont(juce::FontOptions(15.0f));
        g.drawText(juce::String(arranger::label(cp.status)) + " | " + cp.name, 8, 2, width - 16, 28, juce::Justification::centredLeft, true);
        g.setColour(juce::Colour(0xffa9bbce)); g.setFont(juce::FontOptions(13.0f));
        g.drawText(cp.notes, 8, 30, width - 16, 26, juce::Justification::centredLeft, true);
        break;
    }
}
void TaskDetails::selectedRowsChanged(int)
{
    editCheckpoint.setEnabled(selectedCheckpoint() != nullptr && !archivedCheckpoints.getToggleState());
    removeCheckpoint.setEnabled(selectedCheckpoint() != nullptr);
}
void TaskDetails::listBoxItemDoubleClicked(int, const juce::MouseEvent&) { if (!archivedCheckpoints.getToggleState()) promptCheckpoint(false); }
void TaskDetails::promptCheckpoint(bool create)
{
    if (flush().failed()) return;
    const auto* selected = selectedCheckpoint();
    if (!create && !selected) return;
    const auto expected = create ? arranger::TaskCheckpoint{} : *selected;
    const auto id = taskId;
    auto* dialog = new juce::AlertWindow(create ? tr("Новый чек-поинт") : tr("Изменить чек-поинт"), {}, juce::MessageBoxIconType::NoIcon, this);
    dialog->addTextEditor("name", expected.name, tr("Название"));
    if (!create)
    {
        dialog->addTextEditor("notes", expected.notes, "Notes");
        dialog->addComboBox("status", {"POOL", "TODO", "WIP", "DRAFT", "WAIT", "DONE", "BLOCKED"}, tr("Статус"));
        dialog->getComboBoxComponent("status")->setSelectedId(statusId(expected.status));
    }
    dialog->addButton(tr("Сохранить"), 1, juce::KeyPress(juce::KeyPress::returnKey));
    dialog->addButton(tr("Отмена"), 0, juce::KeyPress(juce::KeyPress::escapeKey));
    juce::Component::SafePointer<TaskDetails> safe(this);
    dialog->enterModalState(true, juce::ModalCallbackFunction::create([safe, dialog, create, expected, id](int choice)
    {
        if (safe == nullptr || choice != 1) return;
        const auto name = dialog->getTextEditorContents("name");
        const auto result = safe->execute([&](arranger::WorkspaceCommands& cmd)
        {
            if (create) { juce::String created; return cmd.createCheckpoint(id, name, created); }
            auto value = expected; value.name = name; value.notes = dialog->getTextEditorContents("notes");
            value.status = statuses[juce::jlimit(1, 7, dialog->getComboBoxComponent("status")->getSelectedId()) - 1];
            return cmd.updateCheckpoint(id, expected, value);
        });
        safe->setStatus(result); safe->refreshCheckpoints();
    }), true);
}
void TaskDetails::archiveCheckpoint()
{
    if (flush().failed()) return;
    const auto* cp = selectedCheckpoint(); if (!cp) return;
    const auto id = taskId, cpId = cp->id; const bool restore = cp->archivedAt.isNotEmpty();
    const auto result = execute([&](arranger::WorkspaceCommands& cmd)
        { return restore ? cmd.restoreCheckpoint(id, cpId) : cmd.archiveCheckpoint(id, cpId); });
    setStatus(result); refreshCheckpoints();
}

#include "BlockEditor.h"

namespace
{
const char* types[] = { "text", "heading", "list", "checklist", "link" };
int typeId(const juce::String& type)
{
    for (int i = 0; i < 5; ++i) if (type == types[i]) return i + 1;
    return 1;
}
}
juce::String BlockEditor::tr(const char* value) { return juce::String::fromUTF8(value); }
BlockEditor::BlockEditor()
{
    for (auto* combo : { &newKind, &kind })
    {
        addAndMakeVisible(combo);
        combo->addItemList({tr("Текст"), tr("Заголовок"), tr("Список"), tr("Чек-лист"), tr("Ссылка")}, 1);
        combo->setSelectedId(1, juce::dontSendNotification);
        combo->addKeyListener(this);
    }
    const char* buttonNames[] = {"+ Блок", "Удалить", "Выше", "Ниже", "Отмена", "Повтор", "Сохранить", "Перечитать", "Открыть"};
    int index = 0;
    for (auto* button : { &add, &remove, &up, &down, &undoButton, &redoButton, &saveButton, &reloadButton, &openLink })
    {
        button->setButtonText(tr(buttonNames[index++]));
        addAndMakeVisible(button);
        button->addKeyListener(this);
    }
    nameLabel.setText(tr("Имя блока"), juce::dontSendNotification);
    typeLabel.setText(tr("Тип"), juce::dontSendNotification);
    textLabel.setText(tr("Текст"), juce::dontSendNotification);
    urlLabel.setText("URL", juce::dontSendNotification);
    hint.setText(tr("Добавьте блок и выберите его для редактирования."), juce::dontSendNotification);
    checked.setButtonText(tr("Отмечено"));
    addAndMakeVisible(checked);
    checked.addKeyListener(this);
    for (auto* label : { &nameLabel, &typeLabel, &textLabel, &urlLabel, &status, &hint })
    {
        addAndMakeVisible(label);
        label->setColour(juce::Label::textColourId, juce::Colour(0xffb8c5d5));
    }
    for (auto* field : { &nameField, &textField, &urlField })
    {
        addAndMakeVisible(field);
        field->setFont(juce::FontOptions(17.0f));
        field->setColour(juce::TextEditor::backgroundColourId, juce::Colour(0xff202832));
        field->setColour(juce::TextEditor::textColourId, juce::Colour(0xffe6edf5));
        field->onTextChange = [this] { stageFields(); };
        field->addKeyListener(this);
    }
    textField.setMultiLine(true);
    textField.setReturnKeyStartsNewLine(true);
    nameField.setTextToShowWhenEmpty(tr("Необязательное имя"), juce::Colour(0xff8b9cad));
    urlField.setTextToShowWhenEmpty("https://...", juce::Colour(0xff8b9cad));
    addAndMakeVisible(list);
    list.setRowHeight(64);
    list.setColour(juce::ListBox::backgroundColourId, juce::Colour(0xff202832));
    list.addKeyListener(this);
    addKeyListener(this);
    add.onClick = [this] { addBlock(); };
    remove.onClick = [this] { deleteBlock(); };
    up.onClick = [this] { moveBlock(-1); };
    down.onClick = [this] { moveBlock(1); };
    undoButton.onClick = [this] { history(false); };
    redoButton.onClick = [this] { history(true); };
    saveButton.onClick = [this] { flush(); };
    reloadButton.onClick = [this]
    {
        stopTimer();
        juce::Component::SafePointer<BlockEditor> safe(this);
        juce::AlertWindow::showOkCancelBox(juce::MessageBoxIconType::QuestionIcon,
            tr("Перечитать страницу"), tr("Отбросить несохранённый ввод и загрузить сохранённые блоки?"),
            tr("Перечитать"), tr("Отмена"), this,
            juce::ModalCallbackFunction::create([safe](int result)
            {
                if (safe == nullptr || result != 1) return;
                const auto loaded = safe->session.reload();
                safe->updateStatus(loaded);
                if (loaded.wasOk()) safe->refreshRows(true);
            }));
    };
    kind.onChange = [this] { stageFields(); showBlock(); };
    checked.onClick = [this] { stageFields(); flush(); };
    openLink.onClick = [this]
    {
        if (flush().failed()) return;
        const auto url = urlField.getText().trim();
        if (url.startsWithIgnoreCase("https://") || url.startsWithIgnoreCase("http://"))
            juce::URL(url).launchInDefaultBrowser();
        else updateStatus(juce::Result::fail(tr("Для открытия нужна ссылка http:// или https://.")));
    };
    refreshRows(true);
}
void BlockEditor::paint(juce::Graphics& g) { g.fillAll(juce::Colour(0xff1d232b)); }
void BlockEditor::resized()
{
    auto area = getLocalBounds();
    auto top = area.removeFromTop(34);
    newKind.setBounds(top.removeFromLeft(126).reduced(2));
    add.setBounds(top.removeFromLeft(86).reduced(2));
    undoButton.setBounds(top.removeFromLeft(82).reduced(2));
    redoButton.setBounds(top.removeFromLeft(82).reduced(2));
    saveButton.setBounds(top.removeFromLeft(100).reduced(2));
    reloadButton.setBounds(top.removeFromLeft(112).reduced(2));
    status.setBounds(area.removeFromBottom(28));
    area.removeFromTop(8);
    auto left = area.removeFromLeft(juce::jmin(260, getWidth() / 3));
    auto actions = left.removeFromBottom(34);
    up.setBounds(actions.removeFromLeft(68).reduced(2));
    down.setBounds(actions.removeFromLeft(68).reduced(2));
    remove.setBounds(actions.reduced(2));
    list.setBounds(left);
    area.removeFromLeft(14);
    hint.setBounds(area.withHeight(70));
    nameLabel.setBounds(area.removeFromTop(24));
    nameField.setBounds(area.removeFromTop(30));
    auto typeRow = area.removeFromTop(38);
    typeLabel.setBounds(typeRow.removeFromLeft(38));
    kind.setBounds(typeRow.removeFromLeft(146).reduced(3));
    checked.setBounds(typeRow.reduced(3));
    textLabel.setBounds(area.removeFromTop(24));
    auto linkRow = area.removeFromBottom(34);
    auto linkLabel = area.removeFromBottom(24);
    urlLabel.setBounds(linkLabel);
    openLink.setBounds(linkRow.removeFromRight(90).reduced(2));
    urlField.setBounds(linkRow.reduced(2));
    textField.setBounds(area.reduced(0, 4));
}
juce::Result BlockEditor::bind(const juce::String& id, Session::Read read, Session::Write write)
{
    stopTimer();
    const auto result = session.bind(id, std::move(read), std::move(write));
    updateStatus(result);
    if (result.failed()) return result;
    if (boundId != id) { selected = -1; selectedId.clear(); boundId = id; }
    refreshRows(!session.dirty());
    if (session.dirty()) startTimer(500);
    return result;
}
juce::Result BlockEditor::flush()
{
    stopTimer();
    const auto result = session.flush();
    updateStatus(result);
    refreshRows(false);
    return result;
}
void BlockEditor::updateStatus(const juce::Result& result)
{
    status.setText(result.failed() ? tr("Не сохранено: ") + result.getErrorMessage()
        : session.dirty() ? tr("Изменения ожидают сохранения…") : tr("Сохранено"), juce::dontSendNotification);
    status.setColour(juce::Label::textColourId, result.failed() ? juce::Colour(0xffffab77) : juce::Colour(0xff9eafc3));
    undoButton.setEnabled(session.canUndo());
    redoButton.setEnabled(session.canRedo());
}
void BlockEditor::timerCallback() { flush(); }
int BlockEditor::getNumRows() { return static_cast<int>(session.blocks().size()); }
void BlockEditor::paintListBoxItem(int index, juce::Graphics& g, int width, int height, bool selectedRow)
{
    if (index < 0 || index >= getNumRows()) return;
    const auto& block = session.blocks()[static_cast<size_t>(index)];
    if (selectedRow) { g.setColour(juce::Colour(0xff33475f)); g.fillRoundedRectangle(2, 1, static_cast<float>(width - 4), static_cast<float>(height - 2), 5); }
    g.setColour(juce::Colour(0xffe6edf5));
    g.setFont(juce::FontOptions(block.type == "heading" ? 18.0f : 15.0f));
    auto title = block.name.isNotEmpty() ? block.name : block.text.upToFirstOccurrenceOf("\n", false, false);
    if (title.isEmpty()) title = tr("Новый блок");
    if (block.type == "checklist") title = (block.checked ? tr("☑ ") : tr("☐ ")) + title;
    if (block.type == "list") title = tr("• ") + title;
    g.drawText(title, 10, 2, width - 20, 30, juce::Justification::centredLeft, true);
    g.setColour(juce::Colour(0xff9eafc3)); g.setFont(juce::FontOptions(13.0f));
    const char* labels[] = {"Текст", "Заголовок", "Список", "Чек-лист", "Ссылка"};
    g.drawText(juce::String(index + 1) + " · " + tr(labels[typeId(block.type) - 1]), 10, 33, width - 20, 24, juce::Justification::centredLeft, true);
}
void BlockEditor::selectedRowsChanged(int index)
{
    if (loading || index == selected) return;
    if (flush().failed()) { loading = true; list.selectRow(selected); loading = false; return; }
    selected = index;
    selectedId = index >= 0 && index < getNumRows() ? session.blocks()[static_cast<size_t>(index)].id : juce::String();
    showBlock();
}
void BlockEditor::refreshRows(bool showFields)
{
    loading = true;
    selected = -1;
    for (int i = 0; i < getNumRows(); ++i)
        if (session.blocks()[static_cast<size_t>(i)].id == selectedId) selected = i;
    if (selected < 0 && getNumRows() > 0) { selected = 0; selectedId = session.blocks()[0].id; }
    if (getNumRows() == 0) selectedId.clear();
    list.updateContent();
    list.deselectAllRows();
    if (selected >= 0) list.selectRow(selected);
    list.repaint();
    loading = false;
    if (showFields) showBlock();
    up.setEnabled(selected > 0); down.setEnabled(selected >= 0 && selected + 1 < getNumRows());
    remove.setEnabled(selected >= 0);
    undoButton.setEnabled(session.canUndo()); redoButton.setEnabled(session.canRedo());
}
void BlockEditor::showBlock()
{
    loading = true;
    const bool exists = selected >= 0 && selected < getNumRows();
    for (auto* component : std::initializer_list<juce::Component*>{ &nameLabel, &nameField, &typeLabel, &kind, &textLabel, &textField })
        component->setVisible(exists);
    hint.setVisible(!exists);
    checked.setVisible(false); urlLabel.setVisible(false); urlField.setVisible(false); openLink.setVisible(false);
    if (exists)
    {
        const auto& block = session.blocks()[static_cast<size_t>(selected)];
        nameField.setText(block.name, false); textField.setText(block.text, false); urlField.setText(block.url, false);
        kind.setSelectedId(typeId(block.type), juce::dontSendNotification);
        checked.setToggleState(block.checked, juce::dontSendNotification);
        checked.setVisible(block.type == "checklist");
        const bool link = block.type == "link";
        for (auto* component : std::initializer_list<juce::Component*>{ &urlLabel, &urlField, &openLink }) component->setVisible(link);
        textField.setFont(juce::FontOptions(block.type == "heading" ? 23.0f : 17.0f));
    }
    loading = false;
    up.setEnabled(selected > 0); down.setEnabled(selected >= 0 && selected + 1 < getNumRows()); remove.setEnabled(exists);
}
void BlockEditor::stageFields()
{
    if (loading || selected < 0 || selected >= getNumRows()) return;
    auto blocks = session.blocks();
    auto& block = blocks[static_cast<size_t>(selected)];
    block.name = nameField.getText(); block.text = textField.getText(); block.url = urlField.getText();
    block.type = types[juce::jlimit(1, 5, kind.getSelectedId()) - 1]; block.checked = checked.getToggleState();
    session.stage(std::move(blocks)); list.repaint();
    updateStatus(juce::Result::ok());
    startTimer(500);
}
void BlockEditor::addBlock()
{
    if (flush().failed()) return;
    auto blocks = session.blocks();
    selectedId = juce::Uuid().toString();
    arranger::WorkspaceBlock block { selectedId, types[juce::jlimit(1, 5, newKind.getSelectedId()) - 1], {}, {}, false, {} };
    blocks.insert(blocks.begin() + (selected >= 0 ? selected + 1 : static_cast<int>(blocks.size())), block);
    session.stage(std::move(blocks));
    flush(); refreshRows(true);
    if (selected >= 0) textField.grabKeyboardFocus();
}
void BlockEditor::deleteBlock()
{
    if (flush().failed() || selected < 0) return;
    auto blocks = session.blocks();
    blocks.erase(blocks.begin() + selected); selectedId.clear();
    session.stage(std::move(blocks)); flush(); refreshRows(true);
}
void BlockEditor::moveBlock(int amount)
{
    if (flush().failed() || selected < 0 || selected + amount < 0 || selected + amount >= getNumRows()) return;
    auto blocks = session.blocks();
    std::swap(blocks[static_cast<size_t>(selected)], blocks[static_cast<size_t>(selected + amount)]);
    session.stage(std::move(blocks)); flush(); refreshRows(true);
}
void BlockEditor::history(bool forwards)
{
    stopTimer();
    const auto result = forwards ? session.redo() : session.undo();
    updateStatus(result);
    if (result.wasOk()) refreshRows(true);
}
bool BlockEditor::keyPressed(const juce::KeyPress& key, juce::Component*)
{
    if (!key.getModifiers().isCtrlDown()) return false;
    const int code = key.getKeyCode();
    if (code == 'Z') { history(key.getModifiers().isShiftDown()); return true; }
    if (code == 'Y') { history(true); return true; }
    if (code == 'S') { flush(); return true; }
    return false;
}

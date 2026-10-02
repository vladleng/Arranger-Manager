#include "WorkspaceShell.h"

namespace
{
juce::String text(const char* value) { return juce::String::fromUTF8(value); }
void rowBackground(juce::Graphics& g, int width, int height, bool selected)
{
    if (selected) { g.setColour(juce::Colour(0xff33475f)); g.fillRoundedRectangle(2.0f, 1.0f, static_cast<float>(width - 4), static_cast<float>(height - 2), 5.0f); }
    g.setColour(juce::Colour(0xffe6edf5));
    g.setFont(juce::FontOptions(15.0f));
}
}

WorkspaceShell::WorkspaceShell(arranger::WorkspaceModel& data, std::unique_ptr<HubEditor> editor,
    Execute executeCommand, juce::String selectedView, std::function<void(juce::String)> saveView)
    : model(data), hub(std::move(editor)), runCommand(std::move(executeCommand)),
      persistView(std::move(saveView)), viewKey(std::move(selectedView))
{
    brand.setText("Arranger Manager", juce::dontSendNotification);
    brand.setFont(juce::FontOptions(19.0f, juce::Font::bold));
    title.setFont(juce::FontOptions(26.0f, juce::Font::bold));
    for (auto* label : { &brand, &title, &breadcrumb, &description, &empty })
    {
        addAndMakeVisible(label);
        label->setColour(juce::Label::textColourId, juce::Colour(0xffdce6f1));
    }
    breadcrumb.setColour(juce::Label::textColourId, juce::Colour(0xff9eafc3));
    description.setColour(juce::Label::textColourId, juce::Colour(0xffb8c5d5));
    addPage.setButtonText(text("+ Страница"));
    addChild.setButtonText(text("+ Вложенная"));
    rename.setButtonText(text("Переименовать"));
    move.setButtonText(text("Переместить"));
    archiveButton.setButtonText(text("В архив"));
    restore.setButtonText(text("Восстановить"));
    openSource.setButtonText(text("Открыть песню"));
    for (auto* button : { &addPage, &addChild, &rename, &move, &archiveButton, &restore, &openSource }) addAndMakeVisible(button);
    for (auto* list : { &nav, &tasks, &archive })
    {
        addAndMakeVisible(list);
        list->setRowHeight(list == &nav ? 34 : 52);
        list->setColour(juce::ListBox::backgroundColourId, juce::Colour(0xff202832));
        list->setMultipleSelectionEnabled(false);
    }
    addAndMakeVisible(pageBody);
    pageBody.setReadOnly(true);
    pageBody.setMultiLine(true);
    pageBody.setFont(juce::FontOptions(17.0f));
    pageBody.setColour(juce::TextEditor::backgroundColourId, juce::Colour(0xff202832));
    pageBody.setColour(juce::TextEditor::textColourId, juce::Colour(0xffdce6f1));
    addAndMakeVisible(hub.get());

    addPage.onClick = [this] { promptPage({}); };
    addChild.onClick = [this] { promptPage(selectedPage()); };
    rename.onClick = [this] { promptPage({}, selectedPage()); };
    move.onClick = [this] { showMoveMenu(selectedPage(), move.getScreenBounds().getBottomLeft()); };
    archiveButton.onClick = [this] { archivePage(selectedPage()); };
    restore.onClick = [this] { restorePage(); };
    openSource.onClick = [this] { openTask(tasks.getSelectedRow()); };
    refresh();
}

void WorkspaceShell::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff1d232b));
    g.setColour(juce::Colour(0xff26313d));
    g.fillRect(0, 0, 240, getHeight());
    g.setColour(juce::Colour(0xff465568));
    g.drawVerticalLine(240, 0.0f, static_cast<float>(getHeight()));
}

void WorkspaceShell::resized()
{
    brand.setBounds(14, 12, 210, 32);
    addPage.setBounds(12, 54, 216, 30);
    nav.setBounds(10, 98, 220, juce::jmax(20, getHeight() - 110));
    auto content = getLocalBounds().withTrimmedLeft(250).reduced(14);
    hub->setBounds(content);
    breadcrumb.setBounds(content.removeFromTop(28));
    title.setBounds(content.removeFromTop(48));
    description.setBounds(content.removeFromTop(32));
    auto actions = content.removeFromTop(36);
    addChild.setBounds(actions.removeFromLeft(124).reduced(2));
    rename.setBounds(actions.removeFromLeft(152).reduced(2));
    move.setBounds(actions.removeFromLeft(144).reduced(2));
    archiveButton.setBounds(actions.removeFromLeft(92).reduced(2));
    restore.setBounds(title.getX(), description.getBottom() + 3, 164, 30);
    openSource.setBounds(title.getX(), description.getBottom() + 3, 180, 30);
    content.removeFromTop(12);
    tasks.setBounds(content);
    archive.setBounds(content);
    pageBody.setBounds(content);
    empty.setBounds(content.withHeight(80));
}

juce::String WorkspaceShell::selectedPage() const
{
    return viewKey.startsWith("page:") ? viewKey.substring(5) : juce::String();
}

void WorkspaceShell::refresh()
{
    const int oldTaskIndex = tasks.getSelectedRow(), oldArchiveIndex = archive.getSelectedRow();
    const juce::String oldTaskId = oldTaskIndex >= 0 && oldTaskIndex < static_cast<int>(taskReferences.size())
        ? taskReferences[static_cast<size_t>(oldTaskIndex)].taskId : juce::String();
    const juce::String oldArchiveId = oldArchiveIndex >= 0 && oldArchiveIndex < static_cast<int>(archived.size())
        ? archived[static_cast<size_t>(oldArchiveIndex)] : juce::String();
    rebuilding = true;
    navigation = {{"view:daw", text("DAW"), 0}, {"view:tasks", text("Все задачи"), 0}};
    for (const auto& row : arranger::WorkspaceQueries::activePages(model))
        if (const auto* page = model.findPage(row.id))
            navigation.push_back({"page:" + page->id, page->title, row.depth});
    archived = arranger::WorkspaceQueries::archivedRoots(model);
    navigation.push_back({"view:archive", text("Архив") + " (" + juce::String(static_cast<int>(archived.size())) + ")", 0});
    int selected = -1;
    for (int i = 0; i < static_cast<int>(navigation.size()); ++i)
        if (navigation[static_cast<size_t>(i)].key == viewKey) selected = i;
    if (selected < 0) { viewKey = "view:daw"; selected = 0; }
    nav.updateContent();
    nav.selectRow(selected);
    nav.repaint();
    taskReferences = arranger::WorkspaceQueries::allTasks(model);
    tasks.updateContent();
    tasks.deselectAllRows();
    for (int i = 0; i < static_cast<int>(taskReferences.size()); ++i)
        if (taskReferences[static_cast<size_t>(i)].taskId == oldTaskId) tasks.selectRow(i);
    tasks.repaint();
    archive.updateContent();
    archive.deselectAllRows();
    for (int i = 0; i < static_cast<int>(archived.size()); ++i)
        if (archived[static_cast<size_t>(i)] == oldArchiveId) archive.selectRow(i);
    archive.repaint();
    rebuilding = false;
    if (persistView) persistView(viewKey);
    showDetails();
}

void WorkspaceShell::select(const juce::String& key)
{
    viewKey = key;
    refresh();
}

void WorkspaceShell::showDetails()
{
    const bool daw = viewKey == "view:daw", allTasks = viewKey == "view:tasks", archivedView = viewKey == "view:archive";
    // Hiding DAW may commit a focused inline editor and replace the model.
    // Resolve page pointers only after that focus/visibility transition.
    hub->setVisible(daw);
    const auto* page = model.findPage(selectedPage());
    const bool ordinary = page != nullptr && page->kind == "page";
    tasks.setVisible(allTasks);
    archive.setVisible(archivedView);
    for (auto* component : std::initializer_list<juce::Component*>{ &title, &breadcrumb, &description })
        component->setVisible(!daw);
    for (auto* button : { &addChild, &rename, &move, &archiveButton }) button->setVisible(ordinary);
    restore.setVisible(archivedView);
    restore.setEnabled(archive.getSelectedRow() >= 0 && archive.getSelectedRow() < static_cast<int>(archived.size()));
    openSource.setVisible(allTasks);
    openSource.setEnabled(tasks.getSelectedRow() >= 0 && tasks.getSelectedRow() < static_cast<int>(taskReferences.size()));
    pageBody.setVisible(ordinary && !page->blocks.empty());
    empty.setVisible(false);
    if (allTasks)
    {
        breadcrumb.setText(text("Рабочее пространство"), juce::dontSendNotification);
        title.setText(text("Все задачи"), juce::dontSendNotification);
        description.setText(text("Задач: ") + juce::String(static_cast<int>(taskReferences.size()))
            + text("  ·  Двойной клик открывает песню-источник"), juce::dontSendNotification);
        if (taskReferences.empty()) { empty.setText(text("Добавьте задачу в каталоге DAW — она появится здесь."), juce::dontSendNotification); empty.setVisible(true); }
    }
    else if (archivedView)
    {
        breadcrumb.setText(text("Рабочее пространство"), juce::dontSendNotification);
        title.setText(text("Архив"), juce::dontSendNotification);
        description.setText(text("Страницы и их содержимое сохраняются. Выберите страницу для восстановления."), juce::dontSendNotification);
        if (archived.empty()) { empty.setText(text("Архив пуст."), juce::dontSendNotification); empty.setVisible(true); }
    }
    else if (ordinary)
    {
        breadcrumb.setText(arranger::WorkspaceQueries::breadcrumb(model, page->id), juce::dontSendNotification);
        title.setText(page->title, juce::dontSendNotification);
        int children = 0;
        for (const auto& item : model.pages)
            if (item.parentPageId == page->id && !arranger::WorkspaceQueries::isArchived(model, item.id)) ++children;
        description.setText(text("Вложенных страниц: ") + juce::String(children), juce::dontSendNotification);
        juce::String body;
        for (const auto& block : page->blocks)
        {
            if (block.type == "checklist") body += block.checked ? text("☑ ") : text("☐ ");
            if (block.type == "list") body += text("• ");
            body += block.text + "\n";
            if (block.type == "link" && block.url.isNotEmpty()) body += block.url + "\n";
            body += "\n";
        }
        pageBody.setText(body, false);
        if (page->blocks.empty()) { empty.setText(text("Страница пока пустая. Создавайте вложенные страницы, чтобы организовать проект."), juce::dontSendNotification); empty.setVisible(true); }
    }
}

bool WorkspaceShell::execute(const arranger::WorkspaceAction& action)
{
    const auto result = runCommand(action);
    if (result.failed()) { report(result); return false; }
    refresh();
    return true;
}
void WorkspaceShell::report(const juce::Result& result)
{
    juce::NativeMessageBox::showMessageBoxAsync(juce::MessageBoxIconType::WarningIcon,
        text("Не удалось изменить страницу"), result.getErrorMessage(), this);
}

void WorkspaceShell::promptPage(const juce::String& parentId, const juce::String& renameId)
{
    const auto* existing = model.findPage(renameId);
    const bool renaming = existing != nullptr;
    const auto currentTitle = renaming ? existing->title : juce::String();
    auto* dialog = new juce::AlertWindow(renaming ? text("Переименовать страницу") : text("Новая страница"),
        text("Название страницы"), juce::MessageBoxIconType::NoIcon, this);
    dialog->addTextEditor("title", currentTitle, text("Название"));
    dialog->addButton(text("Сохранить"), 1, juce::KeyPress(juce::KeyPress::returnKey));
    dialog->addButton(text("Отмена"), 0, juce::KeyPress(juce::KeyPress::escapeKey));
    juce::Component::SafePointer<WorkspaceShell> safe(this);
    dialog->enterModalState(true, juce::ModalCallbackFunction::create([safe, dialog, parentId, renameId](int result)
    {
        if (safe == nullptr || result != 1) return;
        const auto title = dialog->getTextEditorContents("title");
        juce::String id = renameId;
        const bool changed = safe->execute([&](arranger::WorkspaceCommands& commands)
        {
            return renameId.isNotEmpty() ? commands.renamePage(renameId, title) : commands.createPage(parentId, title, id);
        });
        if (changed) safe->select("page:" + id);
    }), true);
}

void WorkspaceShell::showPageMenu(const juce::String& id, juce::Point<int> position)
{
    juce::PopupMenu menu;
    menu.addItem(1, text("Новая вложенная страница"));
    menu.addItem(2, text("Переименовать"));
    menu.addItem(3, text("Переместить"));
    menu.addSeparator();
    menu.addItem(4, text("В архив"));
    juce::Component::SafePointer<WorkspaceShell> safe(this);
    menu.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(&nav).withTargetScreenArea({position.x, position.y, 1, 1}),
        [safe, id, position](int item)
        {
            if (safe == nullptr) return;
            if (item == 1) safe->promptPage(id);
            else if (item == 2) safe->promptPage({}, id);
            else if (item == 3) safe->showMoveMenu(id, position);
            else if (item == 4) safe->archivePage(id);
        });
}

void WorkspaceShell::showMoveMenu(const juce::String& id, juce::Point<int> position)
{
    if (id.isEmpty()) return;
    juce::PopupMenu menu;
    menu.addItem(1, text("На верхний уровень"));
    std::vector<juce::String> targets;
    for (const auto& row : arranger::WorkspaceQueries::activePages(model))
        if (!arranger::WorkspaceQueries::isDescendant(model, row.id, id))
        {
            targets.push_back(row.id);
            menu.addItem(static_cast<int>(targets.size()) + 1, arranger::WorkspaceQueries::breadcrumb(model, row.id));
        }
    juce::Component::SafePointer<WorkspaceShell> safe(this);
    menu.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(this).withTargetScreenArea({position.x, position.y, 1, 1}),
        [safe, id, targets](int item)
        {
            if (safe == nullptr || item <= 0 || item > static_cast<int>(targets.size()) + 1) return;
            const auto parent = item == 1 ? juce::String() : targets[static_cast<size_t>(item - 2)];
            if (safe->execute([&](arranger::WorkspaceCommands& commands) { return commands.movePage(id, parent); }))
                safe->select("page:" + id);
        });
}

void WorkspaceShell::archivePage(const juce::String& id)
{
    if (execute([&](arranger::WorkspaceCommands& commands) { return commands.archivePage(id); })) select("view:archive");
}
void WorkspaceShell::restorePage()
{
    const auto index = archive.getSelectedRow();
    if (index < 0 || index >= static_cast<int>(archived.size())) return;
    const auto id = archived[static_cast<size_t>(index)];
    if (execute([&](arranger::WorkspaceCommands& commands) { return commands.restorePage(id); })) select("page:" + id);
}
void WorkspaceShell::openTask(int index)
{
    if (index < 0 || index >= static_cast<int>(taskReferences.size())) return;
    const auto* song = arranger::WorkspaceQueries::song(model, taskReferences[static_cast<size_t>(index)].songId);
    if (song == nullptr) return;
    const auto path = song->path;
    select("view:daw");
    hub->openCatalogSong(path);
}

int WorkspaceShell::NavModel::getNumRows() { return static_cast<int>(owner.navigation.size()); }
void WorkspaceShell::NavModel::paintListBoxItem(int index, juce::Graphics& g, int width, int height, bool selected)
{
    if (index < 0 || index >= getNumRows()) return;
    rowBackground(g, width, height, selected);
    const auto& row = owner.navigation[static_cast<size_t>(index)];
    const int indent = 10 + juce::jmin(row.depth, 6) * 16;
    g.drawText(row.title, indent, 0, juce::jmax(0, width - indent - 10), height, juce::Justification::centredLeft, true);
}
void WorkspaceShell::NavModel::selectedRowsChanged(int index)
{
    if (!owner.rebuilding && index >= 0 && index < getNumRows()) owner.select(owner.navigation[static_cast<size_t>(index)].key);
}
void WorkspaceShell::NavModel::listBoxItemClicked(int index, const juce::MouseEvent& event)
{
    if (index < 0 || index >= getNumRows() || !event.mods.isPopupMenu()) return;
    const auto key = owner.navigation[static_cast<size_t>(index)].key;
    if (key.startsWith("page:")) owner.showPageMenu(key.substring(5), event.getScreenPosition());
}
void WorkspaceShell::NavModel::listBoxItemDoubleClicked(int index, const juce::MouseEvent&)
{
    if (index < 0 || index >= getNumRows()) return;
    const auto key = owner.navigation[static_cast<size_t>(index)].key;
    if (key.startsWith("page:")) owner.promptPage({}, key.substring(5));
}
int WorkspaceShell::TaskModel::getNumRows() { return static_cast<int>(owner.taskReferences.size()); }
void WorkspaceShell::TaskModel::paintListBoxItem(int index, juce::Graphics& g, int width, int height, bool selected)
{
    if (index < 0 || index >= getNumRows()) return;
    const auto& ref = owner.taskReferences[static_cast<size_t>(index)];
    const auto* task = arranger::WorkspaceQueries::task(owner.model, ref);
    if (task == nullptr) return;
    rowBackground(g, width, height, selected);
    g.drawText(juce::String(arranger::label(task->status)) + "  |  " + task->name, 12, 2, width - 24, 26, juce::Justification::centredLeft, true);
    g.setColour(juce::Colour(0xffa9bbce));
    g.setFont(juce::FontOptions(13.0f));
    g.drawText(arranger::WorkspaceQueries::breadcrumb(owner.model, ref.ownerPageId), 12, 29, width - 24, height - 30, juce::Justification::centredLeft, true);
}
void WorkspaceShell::TaskModel::listBoxItemDoubleClicked(int index, const juce::MouseEvent&) { owner.openTask(index); }
int WorkspaceShell::ArchiveModel::getNumRows() { return static_cast<int>(owner.archived.size()); }
void WorkspaceShell::ArchiveModel::paintListBoxItem(int index, juce::Graphics& g, int width, int height, bool selected)
{
    if (index < 0 || index >= getNumRows()) return;
    rowBackground(g, width, height, selected);
    const auto id = owner.archived[static_cast<size_t>(index)];
    const auto* page = owner.model.findPage(id);
    if (page == nullptr) return;
    g.drawText(page->title, 12, 2, width - 24, 26, juce::Justification::centredLeft, true);
    g.setColour(juce::Colour(0xffa9bbce));
    g.setFont(juce::FontOptions(13.0f));
    g.drawText(arranger::WorkspaceQueries::breadcrumb(owner.model, id), 12, 29, width - 24, height - 30, juce::Justification::centredLeft, true);
}
void WorkspaceShell::ArchiveModel::selectedRowsChanged(int index)
{
    owner.restore.setEnabled(index >= 0 && index < getNumRows());
}

void WorkspaceShell::TaskModel::selectedRowsChanged(int index)
{
    owner.openSource.setEnabled(index >= 0 && index < getNumRows());
}

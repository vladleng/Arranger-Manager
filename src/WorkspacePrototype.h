#pragma once
#include <juce_gui_extra/juce_gui_extra.h>
#include "WorkspaceModel.h"

// Small native editor spike for 0.1e. Production page navigation/editor follows
// in 0.1f/0.1g. This persists only a dedicated ordinary page.
class WorkspacePrototype final : public juce::Component
{
public:
    WorkspacePrototype(arranger::WorkspaceModel& data, std::function<void()> persist)
        : model(data), save(std::move(persist))
    {
        for (auto& page : model.pages)
            if (page.kind == "page" && page.title == "Editor prototype") pageId = page.id;
        if (pageId.isEmpty())
        {
            pageId = juce::Uuid().toString();
            model.pages.push_back({pageId, {}, "Editor prototype", "page", {}, {}});
            page()->blocks.push_back({juce::Uuid().toString(), "heading", "Editor prototype", {}, false});
            page()->blocks.push_back({juce::Uuid().toString(), "text", "Try text, lists, checkboxes and links.", {}, false});
            save();
        }
        for (auto* b : { &add, &up, &down, &undo, &redo }) addAndMakeVisible(b);
        addAndMakeVisible(kind);
        kind.addItemList({"Text", "Heading", "List", "Checklist", "Link"}, 1);
        kind.setSelectedId(1);
        addAndMakeVisible(viewport);
        viewport.setViewedComponent(&rows, false);
        add.onClick = [this]
        {
            remember();
            const char* types[] = {"text", "heading", "list", "checklist", "link"};
            page()->blocks.push_back({juce::Uuid().toString(), types[kind.getSelectedId() - 1], {}, {}, false});
            selected = static_cast<int>(page()->blocks.size()) - 1;
            commit();
        };
        up.onClick = [this] { move(-1); };
        down.onClick = [this] { move(1); };
        undo.onClick = [this] { travel(history, future); };
        redo.onClick = [this] { travel(future, history); };
        rebuild();
        setSize(720, 500);
    }

    juce::String getPageId() const { return pageId; }

    void paint(juce::Graphics& g) override { g.fillAll(juce::Colour(0xff1d232b)); }
    void resized() override
    {
        auto bounds = getLocalBounds().reduced(12);
        auto bar = bounds.removeFromTop(32);
        kind.setBounds(bar.removeFromLeft(140).reduced(2));
        for (auto* b : { &add, &up, &down, &undo, &redo }) b->setBounds(bar.removeFromLeft(95).reduced(2));
        bounds.removeFromTop(10);
        viewport.setBounds(bounds);
        layoutRows();
    }

private:
    struct BlockRow : juce::Component
    {
        juce::Label label;
        juce::TextEditor text, url;
        juce::ToggleButton checked;
        juce::TextButton select { "Select" };
    };

    arranger::WorkspacePage* page() { return model.findPage(pageId); }
    void remember()
    {
        history.push_back(page()->blocks);
        if (history.size() > 100) history.erase(history.begin());
        future.clear();
    }
    void commit() { save(); rebuild(); }
    void move(int amount)
    {
        if (selected < 0 || selected + amount < 0 || selected + amount >= static_cast<int>(page()->blocks.size())) return;
        remember();
        std::swap(page()->blocks[static_cast<size_t>(selected)], page()->blocks[static_cast<size_t>(selected + amount)]);
        selected += amount;
        commit();
    }
    void travel(std::vector<std::vector<arranger::WorkspaceBlock>>& source,
        std::vector<std::vector<arranger::WorkspaceBlock>>& destination)
    {
        if (source.empty()) return;
        destination.push_back(page()->blocks);
        page()->blocks = std::move(source.back());
        source.pop_back();
        selected = -1;
        commit();
    }
    void rebuild()
    {
        blockRows.clear();
        for (size_t index = 0; index < page()->blocks.size(); ++index)
        {
            const auto block = page()->blocks[index];
            auto row = std::make_unique<BlockRow>();
            row->label.setText(block.type, juce::dontSendNotification);
            row->text.setMultiLine(true);
            row->text.setReturnKeyStartsNewLine(true);
            row->text.setFont(juce::Font(juce::FontOptions(block.type == "heading" ? 22.0f : 16.0f)));
            row->text.setText(block.text, false);
            row->url.setText(block.url, false);
            row->checked.setToggleState(block.checked, juce::dontSendNotification);
            auto* raw = row.get();
            row->select.onClick = [this, index] { selected = static_cast<int>(index); };
            row->text.onTextChange = [this, index, raw]
            {
                if (index >= page()->blocks.size() || page()->blocks[index].text == raw->text.getText()) return;
                remember();
                page()->blocks[index].text = raw->text.getText();
                save(); // Do not destroy a focused editor in its callback.
            };
            row->url.onTextChange = [this, index, raw]
            {
                if (index >= page()->blocks.size() || page()->blocks[index].url == raw->url.getText()) return;
                remember();
                page()->blocks[index].url = raw->url.getText();
                save();
            };
            row->checked.onClick = [this, index, raw]
            {
                remember();
                page()->blocks[index].checked = raw->checked.getToggleState();
                save();
            };
            for (juce::Component* component : std::initializer_list<juce::Component*>{
                &row->label, &row->text, &row->select })
                row->addAndMakeVisible(component);
            if (block.type == "checklist") row->addAndMakeVisible(row->checked);
            if (block.type == "link") row->addAndMakeVisible(row->url);
            rows.addAndMakeVisible(row.get());
            blockRows.push_back(std::move(row));
        }
        undo.setEnabled(!history.empty());
        redo.setEnabled(!future.empty());
        layoutRows();
    }
    void layoutRows()
    {
        const int width = juce::jmax(200, viewport.getWidth() - 20);
        rows.setSize(width, juce::jmax(viewport.getHeight(), static_cast<int>(blockRows.size()) * 110));
        int y = 0;
        for (auto& row : blockRows)
        {
            row->setBounds(0, y, width, 104);
            row->label.setBounds(0, 0, 100, 24);
            row->select.setBounds(width - 80, 0, 80, 24);
            row->checked.setBounds(0, 32, 24, 32);
            row->text.setBounds(28, 28, width - 30, row->url.isVisible() ? 40 : 70);
            row->url.setBounds(28, 72, width - 30, 28);
            y += 110;
        }
    }

    arranger::WorkspaceModel& model;
    std::function<void()> save;
    juce::String pageId;
    int selected = -1;
    juce::ComboBox kind;
    juce::TextButton add { "Add block" }, up { "Move up" }, down { "Move down" }, undo { "Undo" }, redo { "Redo" };
    juce::Component rows;
    juce::Viewport viewport;
    std::vector<std::unique_ptr<BlockRow>> blockRows;
    std::vector<std::vector<arranger::WorkspaceBlock>> history, future;
};

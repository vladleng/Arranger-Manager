#pragma once
#include "WorkspaceCommands.h"
#include "WorkspaceStore.h"
#include <functional>

namespace arranger
{
class WorkspaceController
{
public:
    using Action = WorkspaceAction;
    WorkspaceController(WorkspaceModel& value, WorkspaceStore& storage) : model(value), store(storage) {}

    juce::Result execute(const Action& action)
    {
        auto candidate = model;
        WorkspaceCommands commands(candidate);
        auto result = action(commands);
        if (result.failed()) return result;
        result = candidate.validate();
        if (result.failed()) return result;
        result = store.save(candidate);
        if (result.wasOk()) model = std::move(candidate);
        return result;
    }
private:
    WorkspaceModel& model;
    WorkspaceStore& store;
};
}

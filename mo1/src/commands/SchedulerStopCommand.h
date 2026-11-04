#pragma once

#include "core/Command.h"
#include "scheduler/Scheduler.h"

// Stops process generation. The CPU cycle loop is intentionally left running.
class SchedulerStopCommand : public Command
{
public:
    explicit SchedulerStopCommand(Scheduler &scheduler);

    const char *name() const override;
    const char *description() const override;
    bool requiresInitialization() const override;
    void execute(CommandContext &context, const std::string &arguments) override;

private:
    Scheduler &scheduler_;
};

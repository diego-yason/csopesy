#pragma once

#include "core/Command.h"
#include "scheduler/Scheduler.h"

// Starts the pseudo-CPU loop and process generation.
class SchedulerStartCommand : public Command
{
public:
    explicit SchedulerStartCommand(Scheduler &scheduler);

    const char *name() const override;
    const char *description() const override;
    bool requiresInitialization() const override;
    void execute(CommandContext &context, const std::string &arguments) override;

private:
    Scheduler &scheduler_;
};

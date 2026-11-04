#pragma once

#include <chrono>
#include <string>

#include "console/ConsoleState.h"
#include "core/Command.h"
#include "scheduler/Scheduler.h"

// Shows the attached process: its name, pid, and the lines it has printed.
class ProcessSmiCommand : public Command
{
public:
    ProcessSmiCommand(const Scheduler &scheduler, const ConsoleState &console);

    const char *name() const override;
    const char *description() const override;
    bool requiresInitialization() const override;
    void execute(CommandContext &context, const std::string &arguments) override;

private:
    const Scheduler &scheduler_;
    const ConsoleState &console_;
};

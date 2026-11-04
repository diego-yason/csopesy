#pragma once

#include "console/ConsoleState.h"
#include "core/Command.h"
#include "scheduler/Scheduler.h"

// Manages process screens: list processes, create one, or attach to one.
class ScreenCommand : public Command
{
public:
    ScreenCommand(Scheduler &scheduler, ConsoleState &console);

    const char *name() const override;
    const char *description() const override;
    bool requiresInitialization() const override;
    std::vector<std::string> usage() const override;
    void execute(CommandContext &context, const std::string &arguments) override;

private:
    void listProcesses(CommandContext &context) const;
    void createProcess(CommandContext &context, const std::string &name);
    void attachToProcess(CommandContext &context, const std::string &name);
    void enterScreen(CommandContext &context, const std::string &name);
    void printUsage(CommandContext &context) const;

    Scheduler &scheduler_;
    ConsoleState &console_;
};

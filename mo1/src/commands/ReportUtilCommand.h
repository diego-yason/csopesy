#pragma once

#include "core/Command.h"
#include "scheduler/Scheduler.h"

// Prints the full process report and appends it to csopesy-log.txt.
class ReportUtilCommand : public Command
{
public:
    explicit ReportUtilCommand(const Scheduler &scheduler);

    const char *name() const override;
    const char *description() const override;
    bool requiresInitialization() const override;
    void execute(CommandContext &context, const std::string &arguments) override;

private:
    const Scheduler &scheduler_;
};

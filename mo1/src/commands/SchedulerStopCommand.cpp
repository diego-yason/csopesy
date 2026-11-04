#include "commands/SchedulerStopCommand.h"

SchedulerStopCommand::SchedulerStopCommand(Scheduler &scheduler)
    : scheduler_(scheduler)
{
}

const char *SchedulerStopCommand::name() const
{
    return "scheduler-stop";
}

const char *SchedulerStopCommand::description() const
{
    return "stops generating processes (the CPU clock keeps ticking)";
}

bool SchedulerStopCommand::requiresInitialization() const
{
    return true;
}

void SchedulerStopCommand::execute(CommandContext &context, const std::string &arguments)
{
    (void)arguments;

    if (!scheduler_.running())
    {
        context.printLine("Scheduler is not running.");
        return;
    }

    // The CPU loop is deliberately not stopped here; it only runs down when the
    // process exits.
    scheduler_.stop();
    context.printLine("Scheduler stopped. The CPU clock keeps ticking.");
}

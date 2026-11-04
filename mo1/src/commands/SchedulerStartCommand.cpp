#include "commands/SchedulerStartCommand.h"

SchedulerStartCommand::SchedulerStartCommand(Scheduler &scheduler)
    : scheduler_(scheduler)
{
}

const char *SchedulerStartCommand::name() const
{
    return "scheduler-start";
}

const char *SchedulerStartCommand::description() const
{
    return "starts the scheduler and generates processes";
}

bool SchedulerStartCommand::requiresInitialization() const
{
    return true;
}

void SchedulerStartCommand::execute(CommandContext &context, const std::string &arguments)
{
    (void)arguments;

    if (scheduler_.running())
    {
        context.printLine("Scheduler is already running.");
        return;
    }

    scheduler_.start();
    context.printLine("Scheduler started.");
}

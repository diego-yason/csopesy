#include "commands/ProcessSmiCommand.h"

#include <string>

#include "console/TimeFormat.h"
#include "process/Process.h"

ProcessSmiCommand::ProcessSmiCommand(const Scheduler &scheduler, const ConsoleState &console)
    : scheduler_(scheduler),
      console_(console)
{
}

const char *ProcessSmiCommand::name() const
{
    return "process-smi";
}

const char *ProcessSmiCommand::description() const
{
    return "shows the attached process and what it has printed";
}

bool ProcessSmiCommand::requiresInitialization() const
{
    return true;
}

void ProcessSmiCommand::execute(CommandContext &context, const std::string &arguments)
{
    (void)arguments;

    if (!console_.attached())
    {
        context.printLine("process-smi is only available inside a process screen. Use 'screen -r <name>' first.");
        return;
    }

    Scheduler::ProcessDetails details;
    if (!scheduler_.processDetails(console_.attachedProcess(), details))
    {
        context.printLine("The attached process '" + console_.attachedProcess() + "' no longer exists.");
        return;
    }

    context.prepareOutput(static_cast<int>(details.log.size()) + 6);
    context.printLine("Process name: " + details.name);
    context.printLine("Process ID: " + std::to_string(details.pid));
    context.printLine("Current Instruction Line: " + std::to_string(details.currentLine));
    context.printLine("Lines of Code: " + std::to_string(details.totalLines));
    context.printLine("");

    if (details.log.empty())
    {
        context.printLine("(no output yet)");
        return;
    }

    // Only the most recent lines are shown.
    const std::size_t firstShown = details.log.size() > Process::kMaxLogEntries
                                       ? details.log.size() - Process::kMaxLogEntries
                                       : 0;

    for (std::size_t index = firstShown; index < details.log.size(); ++index)
    {
        const ProcessLogEntry &entry = details.log[index];
        context.printLine("(" + formatTimestamp(entry.timestamp) + ") Core:" + std::to_string(entry.core) + " " +
                          entry.message);
    }
}

#include "commands/ScreenCommand.h"

#include <string>
#include <vector>

#include "commands/ScreenReport.h"
#include "process/Process.h"

namespace
{
    std::string trim(const std::string &text)
    {
        const std::size_t first = text.find_first_not_of(" \t\r\n");
        if (first == std::string::npos)
        {
            return std::string();
        }

        const std::size_t last = text.find_last_not_of(" \t\r\n");
        return text.substr(first, last - first + 1);
    }
}

ScreenCommand::ScreenCommand(Scheduler &scheduler, ConsoleState &console)
    : scheduler_(scheduler),
      console_(console)
{
}

const char *ScreenCommand::name() const
{
    return "screen";
}

const char *ScreenCommand::description() const
{
    return "manages process screens (-ls lists processes)";
}

bool ScreenCommand::requiresInitialization() const
{
    return true;
}

std::vector<std::string> ScreenCommand::usage() const
{
    return {"screen -ls", "screen -s <process_name>", "screen -r <process_name>"};
}

void ScreenCommand::execute(CommandContext &context, const std::string &arguments)
{
    const std::string trimmed = trim(arguments);
    if (trimmed.empty())
    {
        context.prepareOutput(3);
        context.printLine("Invalid command: screen requires an argument.");
        printUsage(context);
        return;
    }

    const std::size_t split = trimmed.find_first_of(" \t");
    const std::string option = split == std::string::npos ? trimmed : trimmed.substr(0, split);
    const std::string rest = split == std::string::npos ? std::string() : trim(trimmed.substr(split + 1));

    if (option == "-ls")
    {
        listProcesses(context);
        return;
    }

    if (option == "-s")
    {
        createProcess(context, rest);
        return;
    }

    if (option == "-r")
    {
        attachToProcess(context, rest);
        return;
    }

    context.prepareOutput(3);
    context.printLine("Unknown screen option '" + option + "'.");
    printUsage(context);
}

void ScreenCommand::createProcess(CommandContext &context, const std::string &name)
{
    if (name.empty())
    {
        context.prepareOutput(3);
        context.printLine("Invalid command: screen -s requires a process name.");
        printUsage(context);
        return;
    }

    if (name.find_first_of(" \t") != std::string::npos)
    {
        context.printLine("Invalid process name '" + name + "': names cannot contain spaces.");
        return;
    }

    const Process *created = scheduler_.createNamedProcess(name);
    if (created == nullptr)
    {
        context.printLine("Cannot create process '" + name + "': an earlier process already owns that name.");
        return;
    }

    context.printLine("Created process '" + created->name + "' with pid " + std::to_string(created->pid) + ".");

    // Creating a process kicks off CPU scheduling, even without scheduler-start.
    scheduler_.startCpu();

    // Creating a process drops straight into its screen, exactly like -r.
    enterScreen(context, created->name);
}

void ScreenCommand::listProcesses(CommandContext &context) const
{
    ReportOptions options;
    options.truncate = true;
    const std::vector<std::string> lines = buildScreenReport(scheduler_, options);

    context.prepareOutput(static_cast<int>(lines.size()));
    for (const std::string &line : lines)
    {
        context.printLine(line);
    }
}

void ScreenCommand::attachToProcess(CommandContext &context, const std::string &name)
{
    if (name.empty())
    {
        context.prepareOutput(3);
        context.printLine("Invalid command: screen -r requires a process name.");
        printUsage(context);
        return;
    }

    if (name.find_first_of(" \t") != std::string::npos)
    {
        context.printLine("Invalid process name '" + name + "': names cannot contain spaces.");
        return;
    }

    // A finished process is treated exactly like a missing one: it can no longer be resumed.
    ProcessState state = ProcessState::Ready;
    if (!scheduler_.processState(name, state) || state == ProcessState::Finished)
    {
        context.printLine("Process '" + name + "' not found.");
        return;
    }

    enterScreen(context, name);
}

void ScreenCommand::enterScreen(CommandContext &context, const std::string &name)
{
    // Entering a screen wipes the console so the new context starts clean.
    context.clearScreen();
    console_.attach(name);
}

void ScreenCommand::printUsage(CommandContext &context) const
{
    context.printLine("Usage: screen -ls");
    context.printLine("       screen -s <process_name>");
    context.printLine("       screen -r <process_name>");
}

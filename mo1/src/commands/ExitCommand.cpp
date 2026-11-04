#include "commands/ExitCommand.h"

ExitCommand::ExitCommand(ConsoleState &console)
    : console_(console)
{
}

const char *ExitCommand::name() const
{
    return "exit";
}

const char *ExitCommand::description() const
{
    return "exits the program, or leaves the current process screen";
}

void ExitCommand::execute(CommandContext &context, const std::string &arguments)
{
    (void)arguments;

    if (console_.attached())
    {
        context.printLine("Detached from '" + console_.attachedProcess() + "'.");
        console_.detach();
        return;
    }

    context.printLine("Thank you! Goodbye!");
    context.requestExit();
}

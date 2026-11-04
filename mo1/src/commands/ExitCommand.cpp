#include "commands/ExitCommand.h"

const char *ExitCommand::name() const
{
    return "exit";
}

const char *ExitCommand::description() const
{
    return "exits the program";
}

void ExitCommand::execute(CommandContext &context, const std::string &arguments)
{
    (void)arguments;

    context.printLine("Thank you! Goodbye!");
    context.requestExit();
}

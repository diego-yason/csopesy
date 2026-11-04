#include "commands/HelpCommand.h"

HelpCommand::HelpCommand(const CommandRegistry &registry)
    : registry_(registry)
{
}

const char *HelpCommand::name() const
{
    return "help";
}

const char *HelpCommand::description() const
{
    return "displays the commands and its descriptions";
}

bool HelpCommand::requiresInitialization() const
{
    return true;
}

void HelpCommand::execute(CommandContext &context, const std::string &arguments)
{
    (void)arguments;

    const auto &commands = registry_.commands();
    context.prepareOutput(static_cast<int>(commands.size()) + 1);
    context.printLine("Available commands:");

    for (const auto &command : commands)
    {
        context.printLine(command->name() + std::string(" - ") + command->description());
    }
}

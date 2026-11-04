#include "commands/HelpCommand.h"

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
    return "lists the commands, or shows one command's usage";
}

bool HelpCommand::requiresInitialization() const
{
    // help is one of the two commands available before initialization.
    return false;
}

std::vector<std::string> HelpCommand::usage() const
{
    return {"help [command]"};
}

void HelpCommand::execute(CommandContext &context, const std::string &arguments)
{
    const std::string target = trim(arguments);
    if (!target.empty())
    {
        const Command *command = registry_.find(target);
        if (command == nullptr)
        {
            context.printLine("Unknown command '" + target + "'.");
            return;
        }

        const std::vector<std::string> lines = command->usage();
        context.prepareOutput(static_cast<int>(lines.size()) + 1);
        context.printLine(command->name() + std::string(" - ") + command->description());
        for (const std::string &line : lines)
        {
            context.printLine("  " + line);
        }

        return;
    }

    const auto &commands = registry_.commands();
    context.prepareOutput(static_cast<int>(commands.size()) + 1);
    context.printLine("Available commands:");

    for (const auto &command : commands)
    {
        context.printLine(command->name() + std::string(" - ") + command->description());
    }
}

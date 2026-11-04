#include "core/CommandRegistry.h"

#include <utility>

void CommandRegistry::add(std::unique_ptr<Command> command)
{
    if (!command)
    {
        return;
    }

    if (find(command->name()) != nullptr)
    {
        return;
    }

    commands_.push_back(std::move(command));
}

Command *CommandRegistry::find(const std::string &name) const
{
    for (const auto &command : commands_)
    {
        if (name == command->name())
        {
            return command.get();
        }
    }

    return nullptr;
}

const std::vector<std::unique_ptr<Command>> &CommandRegistry::commands() const
{
    return commands_;
}

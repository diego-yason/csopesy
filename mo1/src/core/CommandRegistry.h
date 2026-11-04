#pragma once

#include <memory>
#include <string>
#include <vector>

#include "core/Command.h"

// Owns every registered command and resolves command names to commands.
class CommandRegistry
{
public:
    // Takes ownership of the command. Null or duplicate names are ignored.
    void add(std::unique_ptr<Command> command);

    // Returns the command registered under `name`, or nullptr.
    Command *find(const std::string &name) const;

    // Registration order, used by the "help" listing.
    const std::vector<std::unique_ptr<Command>> &commands() const;

private:
    std::vector<std::unique_ptr<Command>> commands_;
};

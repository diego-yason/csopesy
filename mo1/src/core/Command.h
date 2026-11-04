#pragma once

#include <string>

#include "core/CommandContext.h"

// Base type for every console command. New commands only need to derive from
// this class and be registered with a CommandRegistry.
class Command
{
public:
    virtual ~Command() = default;

    // Keyword the user types to invoke the command.
    virtual const char *name() const = 0;

    // One-line summary shown by the "help" command.
    virtual const char *description() const = 0;

    // True when the command only works after "initialize" loaded config.txt.
    // Such commands are not recognized before initialization.
    virtual bool requiresInitialization() const
    {
        return false;
    }

    // Runs the command. `arguments` is the trimmed text after the command name.
    virtual void execute(CommandContext &context, const std::string &arguments) = 0;
};

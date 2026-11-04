#pragma once

#include "core/Command.h"

// Asks the command handler to shut down.
class ExitCommand : public Command
{
public:
    const char *name() const override;
    const char *description() const override;
    void execute(CommandContext &context, const std::string &arguments) override;
};

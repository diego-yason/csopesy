#pragma once

#include "console/ConsoleState.h"
#include "core/Command.h"

// Leaves the current process screen when attached; otherwise exits the program.
class ExitCommand : public Command
{
public:
    explicit ExitCommand(ConsoleState &console);

    const char *name() const override;
    const char *description() const override;
    void execute(CommandContext &context, const std::string &arguments) override;

private:
    ConsoleState &console_;
};

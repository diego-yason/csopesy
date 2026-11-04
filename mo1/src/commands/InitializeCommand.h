#pragma once

#include "core/Command.h"
#include "config/SessionState.h"

// Reads config.txt on demand and unlocks the commands that depend on it.
class InitializeCommand : public Command
{
public:
    explicit InitializeCommand(SessionState &session);

    const char *name() const override;
    const char *description() const override;
    void execute(CommandContext &context, const std::string &arguments) override;

private:
    SessionState &session_;
};

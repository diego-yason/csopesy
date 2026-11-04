#pragma once

#include "core/Command.h"
#include "core/CommandRegistry.h"

// Lists every command currently registered with the registry.
class HelpCommand : public Command
{
public:
    explicit HelpCommand(const CommandRegistry &registry);

    const char *name() const override;
    const char *description() const override;
    bool requiresInitialization() const override;
    std::vector<std::string> usage() const override;
    void execute(CommandContext &context, const std::string &arguments) override;

private:
    const CommandRegistry &registry_;
};

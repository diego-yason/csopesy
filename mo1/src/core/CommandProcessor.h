#pragma once

#include <string>

#include "core/CommandContext.h"
#include "core/CommandRegistry.h"
#include "config/SessionState.h"

// Turns a raw console line into a command dispatch.
class CommandProcessor
{
public:
    CommandProcessor(CommandRegistry &registry, const SessionState &session);

    // Text shown to the user before a command is typed.
    static const char *prompt();

    // Parses `line` as "<command> [arguments]" and runs the matching command.
    // Commands that need config.txt are rejected until the session is
    // initialized.
    void process(const std::string &line, CommandContext &context) const;

private:
    CommandRegistry &registry_;
    const SessionState &session_;
};

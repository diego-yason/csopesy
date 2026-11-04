#pragma once

#include <string>

#include "core/CommandProcessor.h"

// Tracks which context the console is in: the main command line or an attached
// process screen. The prompt reflects it.
class ConsoleState
{
public:
    bool attached() const
    {
        return !attached_name_.empty();
    }

    const std::string &attachedProcess() const
    {
        return attached_name_;
    }

    void attach(const std::string &name)
    {
        attached_name_ = name;
    }

    void detach()
    {
        attached_name_.clear();
    }

    // "Command> " in the main context, "<process_name>> " inside a screen.
    std::string prompt() const
    {
        return attached() ? attached_name_ + "> " : std::string(CommandProcessor::prompt());
    }

private:
    std::string attached_name_;
};

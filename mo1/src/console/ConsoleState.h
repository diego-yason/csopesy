#pragma once

#include <mutex>
#include <string>

#include "core/CommandProcessor.h"

// Tracks which context the console is in: the main command line or an attached
// process screen. The prompt reflects it. Reads happen from both the input
// thread and the CPU clock thread (live process output), so a mutex guards it.
class ConsoleState
{
public:
    bool attached() const
    {
        std::lock_guard<std::mutex> lock(mutex_);
        return !attached_name_.empty();
    }

    std::string attachedProcess() const
    {
        std::lock_guard<std::mutex> lock(mutex_);
        return attached_name_;
    }

    void attach(const std::string &name)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        attached_name_ = name;
    }

    void detach()
    {
        std::lock_guard<std::mutex> lock(mutex_);
        attached_name_.clear();
    }

    // "Command> " in the main context, "<process_name>> " inside a screen.
    std::string prompt() const
    {
        std::lock_guard<std::mutex> lock(mutex_);
        return attached_name_.empty() ? std::string(CommandProcessor::prompt()) : attached_name_ + "> ";
    }

private:
    mutable std::mutex mutex_;
    std::string attached_name_;
};

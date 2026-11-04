#pragma once

#include <ostream>
#include <string>

#include "core/CommandContext.h"

// Writes command output to a text stream. No console-specific behavior is
// required, so the handler works with any std::ostream (stdout by default).
class ConsoleCommandContext : public CommandContext
{
public:
    explicit ConsoleCommandContext(std::ostream &output)
        : output_(output)
    {
    }

    void printLine(const std::string &text) override
    {
        output_ << text << '\n';
        output_.flush();
    }

    void prepareOutput(int lineCount) override
    {
        // A plain stream needs no space reservation.
        (void)lineCount;
    }

    void requestExit() override
    {
        exit_requested_ = true;
    }

    bool exitRequested() const override
    {
        return exit_requested_;
    }

private:
    std::ostream &output_;
    bool exit_requested_ = false;
};

#pragma once

#include <atomic>
#include <string>

#include "console/InputHandler.h"
#include "core/CommandContext.h"

// Draws command output through the InputHandler so text, prompts, and the live
// input line all share one cursor.
class ScreenCommandContext : public CommandContext
{
public:
    explicit ScreenCommandContext(InputHandler &input)
        : input_(input)
    {
    }

    void printLine(const std::string &text) override
    {
        input_.writeLine(text.c_str());
    }

    void prepareOutput(int lineCount) override
    {
        input_.prepareLines(lineCount);
    }

    void clearScreen() override
    {
        input_.clearScreen();
    }

    int outputCapacity() const override
    {
        return input_.availableLines();
    }

    void requestExit() override
    {
        exit_requested_.store(true);
    }

    bool exitRequested() const override
    {
        return exit_requested_.load();
    }

private:
    InputHandler &input_;
    // Written by the input thread, read by the main thread.
    std::atomic<bool> exit_requested_{false};
};

#pragma once

#include <string>

// Output and control port handed to commands so they never touch the console
// (or any other I/O layer) directly.
class CommandContext
{
public:
    virtual ~CommandContext() = default;

    // Writes one line of output and advances the output cursor.
    virtual void printLine(const std::string &text) = 0;

    // Reserves output space for lineCount lines before anything is printed.
    virtual void prepareOutput(int lineCount) = 0;

    // Asks the command handler to shut down once the current command returns.
    virtual void requestExit() = 0;

    virtual bool exitRequested() const = 0;
};

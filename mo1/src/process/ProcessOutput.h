#pragma once

#include <ostream>
#include <string>

// Where a running process sends output. A detached process gets a sink that
// discards everything; an attached screen will supply its own implementation.
class ProcessOutput
{
public:
    virtual ~ProcessOutput() = default;

    // Writes one line of process output.
    virtual void writeLine(const std::string &text) = 0;
};

// Sink for processes that are not attached to a screen.
class NullProcessOutput : public ProcessOutput
{
public:
    void writeLine(const std::string &text) override
    {
        (void)text;
    }
};

// Writes process output to a text stream.
class StreamProcessOutput : public ProcessOutput
{
public:
    explicit StreamProcessOutput(std::ostream &output)
        : output_(output)
    {
    }

    void writeLine(const std::string &text) override
    {
        output_ << text << '\n';
        output_.flush();
    }

private:
    std::ostream &output_;
};

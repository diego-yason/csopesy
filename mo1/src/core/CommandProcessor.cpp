#include "core/CommandProcessor.h"

#include <cstddef>

namespace
{
    constexpr const char *kPrompt = "Command> ";

    bool isSeparator(char character)
    {
        return character == ' ' || character == '\t' || character == '\r' || character == '\n';
    }

    std::size_t skipSeparators(const std::string &text, std::size_t position)
    {
        while (position < text.size() && isSeparator(text[position]))
        {
            ++position;
        }

        return position;
    }
}

CommandProcessor::CommandProcessor(CommandRegistry &registry, const SessionState &session)
    : registry_(registry),
      session_(session)
{
}

const char *CommandProcessor::prompt()
{
    return kPrompt;
}

void CommandProcessor::process(const std::string &line, CommandContext &context) const
{
    const std::size_t nameStart = skipSeparators(line, 0);
    if (nameStart == line.size())
    {
        return;
    }

    std::size_t nameEnd = nameStart;
    while (nameEnd < line.size() && !isSeparator(line[nameEnd]))
    {
        ++nameEnd;
    }

    const std::string name = line.substr(nameStart, nameEnd - nameStart);
    const std::size_t argumentStart = skipSeparators(line, nameEnd);
    const std::string arguments = argumentStart < line.size() ? line.substr(argumentStart) : std::string();

    Command *command = registry_.find(name);
    if (command != nullptr && command->requiresInitialization() && !session_.initialized())
    {
        // Before initialization only the commands that do not depend on
        // config.txt (initialize, exit) are recognized.
        command = nullptr;
    }

    if (command == nullptr)
    {
        context.printLine(session_.initialized()
                              ? "Unknown command. Type 'help' for the list of commands."
                              : "Unknown command. Type 'initialize' to load config.txt.");
        return;
    }

    command->execute(context, arguments);
}

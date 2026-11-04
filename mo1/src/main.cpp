#include <iostream>
#include <memory>
#include <string>

#include "commands/ExitCommand.h"
#include "commands/HelpCommand.h"
#include "commands/InitializeCommand.h"
#include "config/SessionState.h"
#include "console/ConsoleCommandContext.h"
#include "core/CommandProcessor.h"
#include "core/CommandRegistry.h"

// Entry point for the command handler. Commands are registered here; the
// processor and help listing pick them up automatically.
int main()
{
    std::cout << "CSOPESY Command Handler" << std::endl;
    std::cout << "Type 'initialize' to load config.txt, or 'exit' to quit." << std::endl;
    std::cout << std::endl;

    SessionState session;
    CommandRegistry registry;
    registry.add(std::make_unique<InitializeCommand>(session));
    registry.add(std::make_unique<HelpCommand>(registry));
    registry.add(std::make_unique<ExitCommand>());

    CommandProcessor processor(registry, session);
    ConsoleCommandContext context(std::cout);

    std::string line;
    std::cout << CommandProcessor::prompt() << std::flush;

    while (std::getline(std::cin, line))
    {
        processor.process(line, context);

        if (context.exitRequested())
        {
            break;
        }

        std::cout << CommandProcessor::prompt() << std::flush;
    }

    return 0;
}

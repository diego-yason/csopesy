#include <chrono>
#include <iostream>
#include <memory>
#include <string>
#include <thread>

#include "commands/ExitCommand.h"
#include "commands/HelpCommand.h"
#include "commands/InitializeCommand.h"
#include "commands/ProcessSmiCommand.h"
#include "commands/ReportUtilCommand.h"
#include "commands/SchedulerStartCommand.h"
#include "commands/SchedulerStopCommand.h"
#include "commands/ScreenCommand.h"
#include "config/SessionState.h"
#include "console/ConsoleState.h"
#include "console/InputHandler.h"
#include "console/ScreenCommandContext.h"
#include "core/CommandProcessor.h"
#include "core/CommandRegistry.h"
#include "cpu/CpuClock.h"
#include "scheduler/Scheduler.h"

namespace
{
    // Lines 0..kReservedLines-1 hold the static banner. The command area starts
    // below it and scrolls once it reaches kMaxLines.
    constexpr int kReservedLines = 4;
    constexpr int kMaxLines = 24;

    int g_cursorLine = kReservedLines;
    int g_cursorColumn = 0;
    InputHandler *g_inputHandler = nullptr;
    ConsoleState *g_consoleState = nullptr;

    // Draws the prompt for the current context (main console or process
    // screen) where the cursor currently is.
    void printPrompt()
    {
        if (g_inputHandler == nullptr || g_consoleState == nullptr)
        {
            return;
        }

        const std::string prompt = g_consoleState->prompt();
        g_inputHandler->preparePrompt();
        g_inputHandler->writeCurrentLine(prompt.c_str());
        g_cursorColumn = static_cast<int>(prompt.size());
    }
}

// Entry point for the command handler. Commands are registered here; the
// processor and help listing pick them up automatically.
int main()
{
    std::cout << "\033[H\033[2J" << std::flush;
    std::cout << "CSOPESY Command Handler" << std::endl;
    std::cout << "Type 'initialize' to load config.txt, or 'exit' to quit." << std::endl;
    std::cout << std::endl;

    SessionState session;
    ConsoleState console;
    // Cycle length is how fast the simulated machine runs. 500 us is roughly a
    // thousand instructions a second; pass std::chrono::microseconds(0) to run
    // flat out, or a larger value to slow the machine down.
    CpuClock cpuClock(CpuClock::kDefaultCycle);
    Scheduler scheduler(cpuClock, session);
    CommandRegistry registry;
    registry.add(std::make_unique<InitializeCommand>(session));
    registry.add(std::make_unique<SchedulerStartCommand>(scheduler));
    registry.add(std::make_unique<SchedulerStopCommand>(scheduler));
    registry.add(std::make_unique<ScreenCommand>(scheduler, console));
    registry.add(std::make_unique<ProcessSmiCommand>(scheduler, console));
    registry.add(std::make_unique<ReportUtilCommand>(scheduler));
    registry.add(std::make_unique<HelpCommand>(registry));
    registry.add(std::make_unique<ExitCommand>(console));

    CommandProcessor processor(registry, session);

    InputHandler inputHandler(&g_cursorColumn, &g_cursorLine, {}, kMaxLines, kReservedLines);
    g_inputHandler = &inputHandler;
    g_consoleState = &console;

    ScreenCommandContext context(inputHandler);

    inputHandler.setEnterHandler([&processor, &inputHandler, &context]()
                                 {
        processor.process(inputHandler.getInput(), context);

        if (!context.exitRequested())
        {
            printPrompt();
        } });

    printPrompt();
    inputHandler.start();

    // The exit command only raises the flag; stopping the handler here keeps
    // the poller thread from trying to join itself.
    while (inputHandler.isRunning() && !context.exitRequested())
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    inputHandler.stop();
    return 0;
}

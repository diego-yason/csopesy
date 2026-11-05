# CSOPESY MO1

## Requirements

- Windows (the console UI uses the Win32 API)
- CMake 3.16+ and a C++17 compiler (MinGW-w64/Ninja or MSVC), or plain `g++` 8+

## Build

From this folder:

```
cmake -S . -B build
cmake --build build
```

Or compile directly without CMake:

```
g++ -std=c++17 -Isrc -o handler.exe src/main.cpp src/console/InputHandler.cpp src/core/CommandRegistry.cpp src/core/CommandProcessor.cpp src/config/Config.cpp src/cpu/CpuClock.cpp src/process/Interpreter.cpp src/process/InstructionGenerator.cpp src/scheduler/Scheduler.cpp src/scheduler/SchedulingPolicy.cpp src/commands/HelpCommand.cpp src/commands/ExitCommand.cpp src/commands/InitializeCommand.cpp src/commands/SchedulerStartCommand.cpp src/commands/SchedulerStopCommand.cpp src/commands/ScreenCommand.cpp src/commands/ScreenReport.cpp src/commands/ProcessSmiCommand.cpp src/commands/ReportUtilCommand.cpp
```

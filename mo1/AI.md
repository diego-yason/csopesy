# CSOPESY MO1 — Command Handler

Console command handler with a modular command system. Commands are small
classes registered in one place; the processor and the `help` listing discover
them automatically.

## Layout

```
mo1/
├── CMakeLists.txt
├── config.txt                      system configuration read by `initialize`
└── src/
    ├── main.cpp                     entry point: registration + input loop
    ├── core/                        command framework
    │   ├── Command.h                command interface
    │   ├── CommandContext.h         output/exit port given to commands
    │   ├── CommandProcessor.h/.cpp  line parsing + dispatch
    │   └── CommandRegistry.h/.cpp   command storage + lookup
    ├── commands/                    command implementations
    │   ├── InitializeCommand.h/.cpp
    │   ├── SchedulerStartCommand.h/.cpp
    │   ├── SchedulerStopCommand.h/.cpp
    │   ├── ScreenCommand.h/.cpp
    │   ├── ScreenReport.h/.cpp
    │   ├── ProcessSmiCommand.h/.cpp
    │   ├── ReportUtilCommand.h/.cpp
    │   ├── HelpCommand.h/.cpp
    │   └── ExitCommand.h/.cpp
    ├── config/                      configuration + session state
    │   ├── Config.h/.cpp            config.txt parser and validator
    │   └── SessionState.h           initialized flag + loaded config
    ├── console/                     console adapter
    │   ├── ConsoleState.h           main-console vs attached-screen state + prompt
    │   ├── InputHandler.h/.cpp      keyboard input + cursor-aware drawing
    │   └── ScreenCommandContext.h   CommandContext that draws on the screen
    ├── cpu/                         pseudo-CPU
    │   └── CpuClock.h/.cpp          cycle counter thread started by the scheduler
    ├── process/                     process model + interpreter
    │   ├── Instruction.h            parsed instruction (opcode + operand)
    │   ├── InstructionGenerator.h/.cpp  builds the fixed PRINT/ADD program
    │   ├── Interpreter.h/.cpp       parses and runs process instructions
    │   ├── Process.h                process state + text/data/heap/stack segments
    │   └── ProcessOutput.h          where a running process writes output
    └── scheduler/                   process creation + CPU dispatch
        ├── Scheduler.h/.cpp         per cycle: wake sleepers, create a batch, run one instruction per core
        └── SchedulingPolicy.h/.cpp  ready-queue policies (FCFS, RR)
```

## Commands

| Command      | Available when        | Description                                            |
|--------------|-----------------------|--------------------------------------------------------|
| `initialize` | always                | reads `config.txt` (`initialize [path]` to override)   |
| `exit`       | always                | exits the program                                      |
| `scheduler-start` | after initialization | starts the CPU clock and generates a process every `batch-process-freq` cycles |
| `scheduler-stop`  | after initialization | stops generating processes; the CPU clock keeps ticking |
| `screen -ls`      | after initialization | CPU utilization as `used cores * 100 / total cores` (whole percent) and core counts, where `Cores used` is the allocation the cycle started with: it is recorded right after dispatch, so a `SLEEP` park or a finish later in the same cycle does not make a core look free; then the running processes and the finished ones; long sections show the first 2, `...`, then the last 1 |
| `screen -s <name>`| after initialization | creates a process with exactly that name, starts CPU scheduling, and enters its screen |
| `screen -r <name>`| after initialization | clears the console and switches into that process's screen; unknown or already-finished names report `Process '<name>' not found.` |
| `process-smi`     | inside a screen       | shows the attached process's name, pid, current instruction line, lines of code, and its latest 7 log lines |
| `report-util`     | after initialization | prints the full report (never truncated) and writes it to `csopesy-log.txt`; between the running and finished processes it adds `Queued processes:` (detach time, or `null`) and `Sleeping processes:` (ending with `(cycles remaining: N)`) |
| `help`       | always                | lists the commands; `help <command>` shows a command's usage |

Until `initialize` succeeds, only `initialize` and `exit` are recognized; every
other input is reported as an unknown command. `initialize` reports why it
failed (missing file, missing key, non-integer value) and leaves the session
uninitialized so it can be retried.

Process names are owned for the whole run: the first process to claim a name
keeps it even after it finishes, and `screen -s` with a taken name is rejected.
Auto-generated `p-<pid>` names fall back to `p-<pid>-a`, then `p-<pid>-a-a`, and
so on until a free name is found.

## Configuration

`config.txt` holds one `key value` pair per line (values may be quoted).
Blank lines and lines starting with `#` are ignored.

```
num-cpu 4
scheduler "rr"
quantum-cycles 5
batch-process-freq 1
min-ins 1000
max-ins 2000
delay-per-exec 0
```

Every value is validated when `initialize` runs; the first offending key aborts
the load and nothing is initialized.

| Key                  | Accepted values                          |
|----------------------|------------------------------------------|
| `num-cpu`            | integer in `[1, 128]`                    |
| `scheduler`          | `"fcfs"` or `"rr"` (case-insensitive)    |
| `quantum-cycles`     | integer in `[1, 2^32]`                   |
| `batch-process-freq` | integer in `[1, 2^32]`                   |
| `min-ins`            | integer in `[1, 2^32]`, `<= max-ins`     |
| `max-ins`            | integer in `[1, 2^32]`, `>= min-ins`     |
| `delay-per-exec`     | integer in `[0, 2^32]`                   |

Values must be whole numbers (a sign followed by digits); anything else fails
with a message naming the key and the allowed range.

`delay-per-exec` is enforced by the CPU: before each instruction a process
busy-waits that many cycles while keeping its core, so an instruction costs
`delay-per-exec + 1` cycles. A value of `0` runs one instruction per cycle.

A process becomes `Finished` on the very cycle its last instruction retires, so
it never lingers on a core showing `cur == total` after its program has run out;
the one exception is a `SLEEP` on the last line, which parks the process until it
wakes and only then finishes it.

## Process instructions

A process's text segment holds parsed `Instruction`s. `Interpreter::parse` turns
one text line into an instruction and `Interpreter::execute` / `step` runs it,
writing through a `ProcessOutput` sink. A detached process gets
`NullProcessOutput`, so its output is dropped until a screen is attached.

| Instruction           | Effect                                                                                       |
|-----------------------|----------------------------------------------------------------------------------------------|
| `PRINT(expr)`         | Prints the value: a quoted string, a variable (undeclared reads as `0`), a number, or any of those joined with `+`; no argument prints `Hello world from <process name>!` (e.g. `Hello world from p-0!`) |
| `DECLARE(var, value)` | Creates `var` in the data segment as a uint16 set to `value` (decimal or `0x` hex, clamped to `[0, 65535]`) |
| `ADD(var1, var2/value, var3/value)` | Sets `var1` to `var2 + var3`; operands may be variables or literals, undeclared variables read as `0`, result is clamped to `[0, 65535]` |
| `SUBTRACT(var1, var2/value, var3/value)` | Sets `var1` to `var2 - var3`; same operand rules as `ADD`, and a negative result clamps to `0` |
| `SLEEP(cycles)`       | Blocks the process for `cycles` cycles (uint8, clamped to `[0, 255]`): state becomes `Waiting` and returns to `Ready` after that many `Process::tickSleep()` calls |
| `FOR([instructions], repeats)` | Repeats the bracketed instruction list `repeats` times (clamped to `[0, 65535]`); nests up to 3 levels |

Opcode names are case-insensitive, and whitespace around operands is trimmed.
`DECLARE` variables live in `Process::data.variables`; declaring the same name
twice overwrites the earlier value. Out-of-range literals are clamped rather
than rejected (`-5` becomes `0`, `70000` becomes `65535`); only non-numeric text
is an error.

### What generated processes contain

Every process is created with `x = 0` already in its data segment
(`Scheduler::createProcessLocked`), and `min-ins`/`max-ins` pick the exact number
of instruction lines. The program is always the alternating set

```
PRINT("Value from: " + x)
ADD(x, x, n)        n randomized in [1, 10] on every ADD
PRINT("Value from: " + x)
ADD(x, x, n)
...
```

starting with `PRINT`, so a program of `X` instructions ends with `PRINT` when `X`
is odd and with `ADD` when it is even. The previous random generator (PRINT /
DECLARE / ADD / SUBTRACT / SLEEP with nested `FOR`) is preserved as comments in
`InstructionGenerator.cpp`.

To add an instruction:

1. Add a value to `OpCode` in `core/Instruction.h`.
2. Recognize its name in `Interpreter::parse` and store its operands.
3. Handle it in `Interpreter::execute`.

## Simulation speed

One CPU cycle lasts `CpuClock::kDefaultCycle` (500 µs), so the machine retires
roughly a thousand instructions a second. The clock waits on a
high-resolution waitable timer, because a plain `sleep_for` rounds every wait up
to the Windows timer tick (~15.6 ms) and made the simulation crawl at ~64 cycles
a second.

To change the speed, pass a different cycle length where `main.cpp` builds the
clock:

```cpp
CpuClock cpuClock(std::chrono::microseconds(250)); // about twice as fast
CpuClock cpuClock(std::chrono::microseconds(0));   // flat out, uses a whole core
```

## Build

With CMake (from `mo1/`):

```
cmake -S . -B build
cmake --build build
```

Direct compile (from `mo1/`):

```
g++ -std=c++17 -Isrc src/main.cpp src/console/InputHandler.cpp src/core/CommandRegistry.cpp src/core/CommandProcessor.cpp src/config/Config.cpp src/cpu/CpuClock.cpp src/process/Interpreter.cpp src/process/InstructionGenerator.cpp src/scheduler/Scheduler.cpp src/scheduler/SchedulingPolicy.cpp src/commands/HelpCommand.cpp src/commands/ExitCommand.cpp src/commands/InitializeCommand.cpp src/commands/SchedulerStartCommand.cpp src/commands/SchedulerStopCommand.cpp src/commands/ScreenCommand.cpp src/commands/ScreenReport.cpp src/commands/ProcessSmiCommand.cpp src/commands/ReportUtilCommand.cpp -o handler.exe
```

## Run

```
.\build\csopesy_mo1.exe
```

or `.\handler.exe` after a direct compile.

The banner at the top of the console (`kReservedLines` in `src/main.cpp`) is
redrawn by nothing and never scrolls: the command area below it is a 20-line
scrolling region (`kMaxLines = kReservedLines + kCommandLines`). If you add or
remove a banner line, adjust `kReservedLines` to match, otherwise the prompt
overwrites the banner or leaves a gap.

## Adding a command

1. Add `src/commands/MyCommand.h` / `.cpp` deriving from `Command`
   (`core/Command.h`) and override `name()`, `description()`, and `execute()`.
   Override `requiresInitialization()` to return `true` when the command needs
   the config loaded by `initialize` (defaults to `false`).
2. Register it in `src/main.cpp`:
   `registry.add(std::make_unique<MyCommand>());`
3. Add the new `.cpp` to `CMakeLists.txt` (or to the direct g++ command).

Commands only receive a `CommandContext`, so they never depend on the console
or on any other output mechanism.

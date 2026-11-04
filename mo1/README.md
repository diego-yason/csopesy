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
    │   ├── HelpCommand.h/.cpp
    │   └── ExitCommand.h/.cpp
    ├── config/                      configuration + session state
    │   ├── Config.h/.cpp            config.txt parser and validator
    │   └── SessionState.h           initialized flag + loaded config
    ├── console/                     console adapter
    │   └── ConsoleCommandContext.h  std::ostream implementation of CommandContext
    └── process/                     process model + interpreter
        ├── Instruction.h            parsed instruction (opcode + operand)
        ├── Interpreter.h/.cpp       parses and runs process instructions
        ├── Process.h                process state + text/data/heap/stack segments
        └── ProcessOutput.h          where a running process writes output
```

## Commands

| Command      | Available when        | Description                                            |
|--------------|-----------------------|--------------------------------------------------------|
| `initialize` | always                | reads `config.txt` (`initialize [path]` to override)   |
| `exit`       | always                | exits the program                                      |
| `help`       | after initialization  | lists the commands and their descriptions              |

Until `initialize` succeeds, only `initialize` and `exit` are recognized; every
other input is reported as an unknown command. `initialize` reports why it
failed (missing file, missing key, non-integer value) and leaves the session
uninitialized so it can be retried.

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

## Process instructions

A process's text segment holds parsed `Instruction`s. `Interpreter::parse` turns
one text line into an instruction and `Interpreter::execute` / `step` runs it,
writing through a `ProcessOutput` sink. A detached process gets
`NullProcessOutput`, so its output is dropped until a screen is attached.

| Instruction           | Effect                                                                                       |
|-----------------------|----------------------------------------------------------------------------------------------|
| `PRINT(msg)`          | Prints `msg`; with no argument prints `Hello world from process_N!`                          |
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

To add an instruction:

1. Add a value to `OpCode` in `core/Instruction.h`.
2. Recognize its name in `Interpreter::parse` and store its operands.
3. Handle it in `Interpreter::execute`.

## Build

With CMake (from `mo1/`):

```
cmake -S . -B build
cmake --build build
```

Direct compile (from `mo1/`):

```
g++ -std=c++17 -Isrc src/main.cpp src/core/CommandRegistry.cpp src/core/CommandProcessor.cpp src/config/Config.cpp src/process/Interpreter.cpp src/commands/HelpCommand.cpp src/commands/ExitCommand.cpp src/commands/InitializeCommand.cpp -o handler.exe
```

## Run

```
.\build\csopesy_mo1.exe
```

or `.\handler.exe` after a direct compile.

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

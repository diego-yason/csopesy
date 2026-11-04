#pragma once

#include <string>

#include "process/Instruction.h"
#include "process/Process.h"
#include "process/ProcessOutput.h"

// Turns text lines into instructions and runs them against a process.
class Interpreter
{
public:
    // Parses one text line such as "PRINT(Hello)". Returns false and fills
    // `errorMessage` when the line is not a valid instruction.
    static bool parse(const std::string &text, Instruction &instruction, std::string &errorMessage);

    // Runs a single instruction for `process`, sending output to `output`.
    static void execute(Process &process, const Instruction &instruction, ProcessOutput &output);

    // Runs the next instruction in the process's text segment and advances its
    // instruction pointer. Returns false when no instructions are left.
    static bool step(Process &process, ProcessOutput &output);
};

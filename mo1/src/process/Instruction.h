#pragma once

#include <string>
#include <vector>

// Operations the interpreter understands. New opcodes are added here and
// handled in Interpreter::execute.
enum class OpCode
{
    Print,    // PRINT(msg): shows the message on the process's screen
    Declare,  // DECLARE(var, value): creates a uint16 in the data segment
    Add,      // ADD(var1, var2/value, var3/value): var1 = var2 + var3
    Subtract, // SUBTRACT(var1, var2/value, var3/value): var1 = var2 - var3
    Sleep,    // SLEEP(cycles): blocks the process for a uint8 number of cycles
    For       // FOR([instructions], repeats): repeats the body `repeats` times
};

// One executable step of a process, parsed from its text segment.
struct Instruction
{
    OpCode opcode = OpCode::Print;

    // Operand text in source order. Opcodes without operands leave this empty;
    // PRINT keeps its whole message as a single operand.
    std::vector<std::string> operands;

    // Instructions the FOR repeats; empty for every other opcode.
    std::vector<Instruction> body;
};

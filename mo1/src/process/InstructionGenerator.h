#pragma once

#include <cstddef>
#include <random>
#include <string>
#include <vector>

#include "process/Instruction.h"

// Builds the instruction program for generated processes. Every program is the
// alternating set PRINT("Value from: " + x), ADD(x, x, [1-10]), PRINT, ... and
// the requested line count is met exactly.
class InstructionGenerator
{
public:
    // `seed` lets a run be reproduced.
    explicit InstructionGenerator(unsigned int seed = std::random_device{}());

    // Generates a program of between `minLines` and `maxLines` instruction
    // lines (inclusive). Returns an empty program when `maxLines` is 0.
    std::vector<Instruction> generate(std::size_t minLines, std::size_t maxLines);

private:
    // Previous random-program helpers. They are unused now; their bodies are
    // kept as comments in InstructionGenerator.cpp for reference.
    std::vector<Instruction> generateBlock(std::size_t lineCount, std::size_t depth);

    Instruction makeRandomInstruction();
    Instruction makePrint();
    Instruction makeDeclare();
    Instruction makeArithmetic(OpCode opcode);
    Instruction makeSleep();
    Instruction makeLoop(std::size_t bodyLines, std::size_t depth);
    std::size_t makeLoopSize(std::size_t lineCount);

    std::string nextVariableName();
    std::string pickVariable();
    std::string randomOperand();
    unsigned int randomValue();

    bool chance(int percent);

    static constexpr std::size_t kMaxForDepth = 3;

    std::mt19937 rng_;
    std::vector<std::string> variables_;
    std::size_t nextVariableIndex_ = 0;
};

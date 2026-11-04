#include "process/InstructionGenerator.h"

#include <algorithm>
#include <utility>

namespace
{
    constexpr int kPrintPercent = 30;
    constexpr int kDeclarePercent = 20;
    constexpr int kAddPercent = 20;
    constexpr int kSubtractPercent = 15;
    // Anything left is SLEEP.

    constexpr int kVariableOperandPercent = 60;
    constexpr int kLoopPercent = 20;
    constexpr std::size_t kMaxLoopBody = 4;
    constexpr unsigned int kMaxRepeat = 5;
    constexpr unsigned int kMaxSleep = 20;
    constexpr unsigned int kMaxValue = 99;
}

InstructionGenerator::InstructionGenerator(unsigned int seed)
    : rng_(seed)
{
}

std::vector<Instruction> InstructionGenerator::generate(std::size_t minLines, std::size_t maxLines)
{
    if (maxLines == 0)
    {
        return {};
    }

    if (minLines == 0)
    {
        minLines = 1;
    }

    if (minLines > maxLines)
    {
        std::swap(minLines, maxLines);
    }

    variables_.clear();
    nextVariableIndex_ = 0;

    std::uniform_int_distribution<std::size_t> lineCount(minLines, maxLines);
    return generateBlock(lineCount(rng_), 1);
}

std::vector<Instruction> InstructionGenerator::generateBlock(std::size_t lineCount, std::size_t depth)
{
    std::vector<Instruction> block;

    while (lineCount > 0)
    {
        if (depth < kMaxForDepth && lineCount >= 2 && chance(kLoopPercent))
        {
            const std::size_t bodyLines = makeLoopSize(lineCount);
            block.push_back(makeLoop(bodyLines, depth));
            lineCount -= bodyLines + 1;
            continue;
        }

        block.push_back(makeRandomInstruction());
        --lineCount;
    }

    return block;
}

Instruction InstructionGenerator::makeRandomInstruction()
{
    std::uniform_int_distribution<int> choice(1, 100);
    const int roll = choice(rng_);

    if (roll <= kPrintPercent)
    {
        return makePrint();
    }

    if (roll <= kPrintPercent + kDeclarePercent)
    {
        return makeDeclare();
    }

    if (roll <= kPrintPercent + kDeclarePercent + kAddPercent)
    {
        return makeArithmetic(OpCode::Add);
    }

    if (roll <= kPrintPercent + kDeclarePercent + kAddPercent + kSubtractPercent)
    {
        return makeArithmetic(OpCode::Subtract);
    }

    return makeSleep();
}

Instruction InstructionGenerator::makePrint()
{
    // Generated programs always use the default PRINT message.
    //
    // The interpreter still understands the full form - quoted strings,
    // variables, numbers, and '+' concatenation - so richer prints can be
    // restored here later without touching parsing or execution.
    Instruction instruction;
    instruction.opcode = OpCode::Print;
    return instruction;
}

Instruction InstructionGenerator::makeDeclare()
{
    Instruction instruction;
    instruction.opcode = OpCode::Declare;
    instruction.operands = {nextVariableName(), std::to_string(randomValue())};
    return instruction;
}

Instruction InstructionGenerator::makeArithmetic(OpCode opcode)
{
    std::string destination;
    if (!variables_.empty() && chance(60))
    {
        destination = pickVariable();
    }
    else
    {
        destination = nextVariableName();
    }

    Instruction instruction;
    instruction.opcode = opcode;
    instruction.operands = {destination, randomOperand(), randomOperand()};
    return instruction;
}

Instruction InstructionGenerator::makeSleep()
{
    std::uniform_int_distribution<unsigned int> cycles(0, kMaxSleep);

    Instruction instruction;
    instruction.opcode = OpCode::Sleep;
    instruction.operands = {std::to_string(cycles(rng_))};
    return instruction;
}

Instruction InstructionGenerator::makeLoop(std::size_t bodyLines, std::size_t depth)
{
    std::uniform_int_distribution<unsigned int> repeat(1, kMaxRepeat);

    Instruction instruction;
    instruction.opcode = OpCode::For;
    instruction.operands = {std::to_string(repeat(rng_))};
    instruction.body = generateBlock(bodyLines, depth + 1);
    return instruction;
}

std::size_t InstructionGenerator::makeLoopSize(std::size_t lineCount)
{
    const std::size_t upper = std::min<std::size_t>(lineCount - 1, kMaxLoopBody);
    std::uniform_int_distribution<std::size_t> bodyLines(1, upper);
    return bodyLines(rng_);
}

std::string InstructionGenerator::nextVariableName()
{
    const std::string name = "var" + std::to_string(nextVariableIndex_++);
    variables_.push_back(name);
    return name;
}

std::string InstructionGenerator::pickVariable()
{
    std::uniform_int_distribution<std::size_t> index(0, variables_.size() - 1);
    return variables_[index(rng_)];
}

std::string InstructionGenerator::randomOperand()
{
    if (!variables_.empty() && chance(kVariableOperandPercent))
    {
        return pickVariable();
    }

    return std::to_string(randomValue());
}

unsigned int InstructionGenerator::randomValue()
{
    std::uniform_int_distribution<unsigned int> value(0, kMaxValue);
    return value(rng_);
}

bool InstructionGenerator::chance(int percent)
{
    std::uniform_int_distribution<int> roll(1, 100);
    return roll(rng_) <= percent;
}

#include "process/Interpreter.h"

#include <cctype>
#include <cstdint>
#include <string>
#include <vector>

// Declared up front because FOR parsing recurses through it.
static bool parseInstruction(const std::string &text,
                             Instruction &instruction,
                             std::string &errorMessage,
                             std::size_t depth);

namespace
{
    std::string trim(const std::string &text)
    {
        std::size_t start = 0;
        while (start < text.size() && std::isspace(static_cast<unsigned char>(text[start])) != 0)
        {
            ++start;
        }

        std::size_t end = text.size();
        while (end > start && std::isspace(static_cast<unsigned char>(text[end - 1])) != 0)
        {
            --end;
        }

        return text.substr(start, end - start);
    }

    std::string toUpper(const std::string &text)
    {
        std::string upper = text;
        for (char &character : upper)
        {
            character = static_cast<char>(std::toupper(static_cast<unsigned char>(character)));
        }

        return upper;
    }

    // Message PRINT shows when the instruction carries no argument.
    std::string defaultPrintMessage(const Process &process)
    {
        return "Hello world from process_" + std::to_string(process.pid) + "!";
    }

    // True for [A-Za-z_][A-Za-z0-9_]*.
    bool isIdentifier(const std::string &text)
    {
        if (text.empty())
        {
            return false;
        }

        const unsigned char first = static_cast<unsigned char>(text.front());
        if (std::isalpha(first) == 0 && text.front() != '_')
        {
            return false;
        }

        for (const char character : text)
        {
            const unsigned char value = static_cast<unsigned char>(character);
            if (std::isalnum(value) == 0 && character != '_')
            {
                return false;
            }
        }

        return true;
    }

    // Largest value a declared variable can hold.
    constexpr unsigned long long kMaxUint16 = 0xFFFFull;

    // Parses a decimal or 0x-prefixed hex literal and clamps it to [0, 65535].
    // Only non-numeric text is an error.
    bool parseUint16(const std::string &text, std::uint16_t &value, std::string &errorMessage)
    {
        std::size_t position = 0;
        bool negative = false;
        if (!text.empty() && (text[0] == '+' || text[0] == '-'))
        {
            negative = text[0] == '-';
            position = 1;
        }

        int base = 10;
        if (text.size() > position + 2 && text[position] == '0' &&
            (text[position + 1] == 'x' || text[position + 1] == 'X'))
        {
            base = 16;
            position += 2;
        }

        if (position >= text.size())
        {
            errorMessage = "Value '" + text + "' is not a whole number.";
            return false;
        }

        // Accumulate saturating at kMaxUint16 + 1, so arbitrarily long literals
        // never overflow and simply clamp.
        unsigned long long magnitude = 0;
        for (std::size_t index = position; index < text.size(); ++index)
        {
            const unsigned char character = static_cast<unsigned char>(text[index]);

            unsigned int digit = 0;
            if (character >= '0' && character <= '9')
            {
                digit = character - '0';
            }
            else if (base == 16 && std::isxdigit(character) != 0)
            {
                digit = static_cast<unsigned int>(std::tolower(character) - 'a' + 10);
            }
            else
            {
                errorMessage = "Value '" + text + "' is not a whole number.";
                return false;
            }

            magnitude = magnitude * static_cast<unsigned long long>(base) + digit;
            if (magnitude > kMaxUint16)
            {
                magnitude = kMaxUint16 + 1;
            }
        }

        if (negative)
        {
            value = 0;
            return true;
        }

        value = static_cast<std::uint16_t>(magnitude > kMaxUint16 ? kMaxUint16 : magnitude);
        return true;
    }

    // Splits comma-separated operands and trims each one.
    std::vector<std::string> splitOperands(const std::string &text)
    {
        std::vector<std::string> operands;
        std::size_t start = 0;
        while (true)
        {
            const std::size_t comma = text.find(',', start);
            if (comma == std::string::npos)
            {
                operands.push_back(trim(text.substr(start)));
                break;
            }

            operands.push_back(trim(text.substr(start, comma - start)));
            start = comma + 1;
        }

        return operands;
    }

    // Reads an operand that may be a variable name or a literal. Undeclared
    // variables read as 0.
    std::uint16_t resolveOperand(const Process &process, const std::string &operand)
    {
        if (isIdentifier(operand))
        {
            const auto found = process.data.variables.find(operand);
            return found == process.data.variables.end() ? 0 : found->second;
        }

        std::uint16_t value = 0;
        std::string errorMessage;
        parseUint16(operand, value, errorMessage);
        return value;
    }

    // True when `text` is a variable name or a whole-number literal.
    bool isValueOperand(const std::string &text)
    {
        if (isIdentifier(text))
        {
            return true;
        }

        std::uint16_t value = 0;
        std::string errorMessage;
        return parseUint16(text, value, errorMessage);
    }

    std::uint16_t clampToUint16(unsigned long long value)
    {
        return static_cast<std::uint16_t>(value > kMaxUint16 ? kMaxUint16 : value);
    }

    // Parses a sleep duration and clamps it to a uint8.
    bool parseSleepCycles(const std::string &text, std::uint8_t &cycles, std::string &errorMessage)
    {
        std::uint16_t value = 0;
        if (!parseUint16(text, value, errorMessage))
        {
            return false;
        }

        cycles = static_cast<std::uint8_t>(value > 0xFF ? 0xFF : value);
        return true;
    }

    // Parses "NAME(dest, left, right)" for the arithmetic opcodes.
    bool parseArithmetic(const std::string &name,
                         const std::string &inner,
                         OpCode opcode,
                         Instruction &instruction,
                         std::string &errorMessage)
    {
        const std::vector<std::string> operands = splitOperands(inner);
        if (operands.size() != 3)
        {
            errorMessage = name + " expects three arguments: " + name + "(var1, var2/value, var3/value).";
            return false;
        }

        if (!isIdentifier(operands[0]))
        {
            errorMessage = name + " destination '" + operands[0] + "' is not a valid identifier.";
            return false;
        }

        for (std::size_t index = 1; index < operands.size(); ++index)
        {
            if (!isValueOperand(operands[index]))
            {
                errorMessage = name + " operand '" + operands[index] + "' is neither a variable nor a whole number.";
                return false;
            }
        }

        instruction.opcode = opcode;
        instruction.operands = operands;
        return true;
    }

    // Deepest FOR nesting the parser accepts.
    constexpr std::size_t kMaxForDepth = 3;

    // Splits on commas that are not nested inside (...) or [...].
    std::vector<std::string> splitTopLevel(const std::string &text)
    {
        std::vector<std::string> parts;
        int depth = 0;
        std::size_t start = 0;
        for (std::size_t index = 0; index < text.size(); ++index)
        {
            const char character = text[index];
            if (character == '(' || character == '[')
            {
                ++depth;
            }
            else if (character == ')' || character == ']')
            {
                --depth;
            }
            else if (character == ',' && depth == 0)
            {
                parts.push_back(trim(text.substr(start, index - start)));
                start = index + 1;
            }
        }

        parts.push_back(trim(text.substr(start)));
        return parts;
    }

    // Parses "[instr, instr, ...], repeats".
    bool parseFor(const std::string &inner, Instruction &instruction, std::string &errorMessage, std::size_t depth)
    {
        if (depth > kMaxForDepth)
        {
            errorMessage = "FOR can be nested at most " + std::to_string(kMaxForDepth) + " levels deep.";
            return false;
        }

        if (inner.empty() || inner.front() != '[')
        {
            errorMessage = "FOR expects a bracketed body: FOR([instructions], repeats).";
            return false;
        }

        std::size_t close = std::string::npos;
        int bracketDepth = 0;
        for (std::size_t index = 0; index < inner.size(); ++index)
        {
            if (inner[index] == '[')
            {
                ++bracketDepth;
            }
            else if (inner[index] == ']')
            {
                --bracketDepth;
                if (bracketDepth == 0)
                {
                    close = index;
                    break;
                }
            }
        }

        if (close == std::string::npos)
        {
            errorMessage = "FOR body is missing a closing ']'.";
            return false;
        }

        const std::string bodyText = inner.substr(1, close - 1);
        const std::string rest = trim(inner.substr(close + 1));
        if (rest.empty() || rest.front() != ',')
        {
            errorMessage = "FOR expects a repeat count after the body: FOR([instructions], repeats).";
            return false;
        }

        const std::string repeatsText = trim(rest.substr(1));
        std::uint16_t repeats = 0;
        std::string repeatsError;
        if (repeatsText.empty() || !parseUint16(repeatsText, repeats, repeatsError))
        {
            errorMessage = "FOR repeats must be a whole number, got '" + repeatsText + "'.";
            return false;
        }

        if (trim(bodyText).empty())
        {
            errorMessage = "FOR body cannot be empty.";
            return false;
        }

        std::vector<Instruction> body;
        for (const std::string &part : splitTopLevel(bodyText))
        {
            if (part.empty())
            {
                errorMessage = "FOR body contains an empty instruction.";
                return false;
            }

            Instruction nested;
            if (!parseInstruction(part, nested, errorMessage, depth + 1))
            {
                return false;
            }

            body.push_back(nested);
        }

        instruction.opcode = OpCode::For;
        instruction.operands = {repeatsText};
        instruction.body = body;
        return true;
    }

    // Repeat count of a FOR instruction; 0 when the operand is missing or invalid.
    std::size_t repeatCount(const Instruction &instruction)
    {
        if (instruction.operands.empty())
        {
            return 0;
        }

        std::uint16_t repeats = 0;
        std::string errorMessage;
        if (!parseUint16(instruction.operands[0], repeats, errorMessage))
        {
            return 0;
        }

        return static_cast<std::size_t>(repeats);
    }

    // The instruction list that the last index of `position` points into.
    const std::vector<Instruction> *listFor(const std::vector<Instruction> &root,
                                            const std::vector<std::size_t> &position)
    {
        const std::vector<Instruction> *list = &root;
        for (std::size_t index = 0; index + 1 < position.size(); ++index)
        {
            if (position[index] >= list->size())
            {
                return nullptr;
            }

            list = &(*list)[position[index]].body;
        }

        return list;
    }
}

// Parses one instruction line. `depth` is 1 for top-level instructions and
// grows for each FOR body level.
static bool parseInstruction(const std::string &text,
                             Instruction &instruction,
                             std::string &errorMessage,
                             std::size_t depth)
{
    const std::string trimmed = trim(text);
    if (trimmed.empty())
    {
        errorMessage = "Empty instruction.";
        return false;
    }

    const std::size_t openParen = trimmed.find('(');
    const std::string name = toUpper(trim(openParen == std::string::npos ? trimmed : trimmed.substr(0, openParen)));

    std::string inner;
    if (openParen != std::string::npos)
    {
        if (trimmed.back() != ')')
        {
            errorMessage = "Instruction '" + trimmed + "' is missing a closing ')'.";
            return false;
        }

        inner = trim(trimmed.substr(openParen + 1, trimmed.size() - openParen - 2));
    }
    else if (trimmed.find(')') != std::string::npos)
    {
        errorMessage = "Instruction '" + trimmed + "' has no opening '('.";
        return false;
    }

    if (name == "PRINT")
    {
        // The whole message is one operand, so commas inside it are kept.
        instruction.opcode = OpCode::Print;
        instruction.operands.clear();
        if (!inner.empty())
        {
            instruction.operands.push_back(inner);
        }
        return true;
    }

    if (name == "DECLARE")
    {
        const std::vector<std::string> operands = splitOperands(inner);
        if (operands.size() != 2)
        {
            errorMessage = "DECLARE expects two arguments: DECLARE(var, value).";
            return false;
        }

        if (!isIdentifier(operands[0]))
        {
            errorMessage = "DECLARE variable name '" + operands[0] + "' is not a valid identifier.";
            return false;
        }

        std::uint16_t value = 0;
        std::string valueError;
        if (!parseUint16(operands[1], value, valueError))
        {
            errorMessage = "DECLARE value must be a whole number, got '" + operands[1] + "'.";
            return false;
        }

        instruction.opcode = OpCode::Declare;
        instruction.operands = operands;
        return true;
    }

    if (name == "ADD" || name == "SUBTRACT")
    {
        return parseArithmetic(name, inner, name == "ADD" ? OpCode::Add : OpCode::Subtract, instruction, errorMessage);
    }

    if (name == "SLEEP")
    {
        const std::vector<std::string> operands = splitOperands(inner);
        if (operands.size() != 1 || operands[0].empty())
        {
            errorMessage = "SLEEP expects one argument: SLEEP(cycles).";
            return false;
        }

        std::uint8_t cycles = 0;
        std::string cyclesError;
        if (!parseSleepCycles(operands[0], cycles, cyclesError))
        {
            errorMessage = "SLEEP cycles must be a whole number, got '" + operands[0] + "'.";
            return false;
        }

        instruction.opcode = OpCode::Sleep;
        instruction.operands = operands;
        return true;
    }

    if (name == "FOR")
    {
        return parseFor(inner, instruction, errorMessage, depth);
    }

    errorMessage = "Unknown instruction '" + trimmed + "'.";
    return false;
}

bool Interpreter::parse(const std::string &text, Instruction &instruction, std::string &errorMessage)
{
    return parseInstruction(text, instruction, errorMessage, 1);
}

void Interpreter::execute(Process &process, const Instruction &instruction, ProcessOutput &output)
{
    switch (instruction.opcode)
    {
    case OpCode::Print:
    {
        const bool hasMessage = !instruction.operands.empty() && !instruction.operands[0].empty();
        output.writeLine(hasMessage ? instruction.operands[0] : defaultPrintMessage(process));
        return;
    }

    case OpCode::Declare:
    {
        if (instruction.operands.size() != 2)
        {
            return;
        }

        std::uint16_t value = 0;
        std::string errorMessage;
        if (!parseUint16(instruction.operands[1], value, errorMessage))
        {
            return;
        }

        process.data.variables[instruction.operands[0]] = value;
        return;
    }

    case OpCode::Add:
    {
        if (instruction.operands.size() != 3)
        {
            return;
        }

        const unsigned long long sum = static_cast<unsigned long long>(resolveOperand(process, instruction.operands[1])) +
                                       static_cast<unsigned long long>(resolveOperand(process, instruction.operands[2]));

        process.data.variables[instruction.operands[0]] = clampToUint16(sum);
        return;
    }

    case OpCode::Subtract:
    {
        if (instruction.operands.size() != 3)
        {
            return;
        }

        const long long difference = static_cast<long long>(resolveOperand(process, instruction.operands[1])) -
                                     static_cast<long long>(resolveOperand(process, instruction.operands[2]));

        // uint16 cannot hold a negative value, so underflow clamps to 0.
        process.data.variables[instruction.operands[0]] = static_cast<std::uint16_t>(difference < 0 ? 0 : difference);
        return;
    }

    case OpCode::Sleep:
    {
        if (instruction.operands.size() != 1)
        {
            return;
        }

        std::uint8_t cycles = 0;
        std::string errorMessage;
        if (!parseSleepCycles(instruction.operands[0], cycles, errorMessage))
        {
            return;
        }

        if (cycles == 0)
        {
            return;
        }

        process.sleepCycles = cycles;
        process.state = ProcessState::Waiting;
        return;
    }

    case OpCode::For:
        // Loops are driven by step(), which moves the cursor into the body.
        return;
    }
}

bool Interpreter::step(Process &process, ProcessOutput &output)
{
    const std::vector<Instruction> &root = process.text.instructions;
    if (root.empty())
    {
        return false;
    }

    if (process.position.empty())
    {
        process.position.push_back(0);
    }

    while (true)
    {
        const std::vector<Instruction> *list = listFor(root, process.position);
        if (list == nullptr)
        {
            return false;
        }

        if (process.position.back() >= list->size())
        {
            // The current instruction list ran out.
            if (process.position.size() == 1)
            {
                return false; // the program finished
            }

            if (process.remainingIterations.back() > 0)
            {
                --process.remainingIterations.back();
                process.position.back() = 0;
                continue;
            }

            // The innermost loop is done; resume after its FOR.
            process.remainingIterations.pop_back();
            process.position.pop_back();
            ++process.position.back();
            continue;
        }

        const Instruction &instruction = (*list)[process.position.back()];

        if (instruction.opcode == OpCode::For)
        {
            const std::size_t repeats = repeatCount(instruction);
            if (repeats == 0)
            {
                ++process.position.back();
                return true;
            }

            // Keep the parent index on the FOR so leaving the loop advances
            // past it, then descend into the body.
            process.remainingIterations.push_back(repeats - 1);
            process.position.push_back(0);
            return true;
        }

        ++process.position.back();
        execute(process, instruction, output);
        return true;
    }
}

#include "config/Config.h"

#include <cctype>
#include <fstream>
#include <map>
#include <stdexcept>
#include <utility>

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

    std::string stripQuotes(const std::string &text)
    {
        if (text.size() >= 2 && text.front() == '"' && text.back() == '"')
        {
            return text.substr(1, text.size() - 2);
        }

        return text;
    }

    // True when `text` is an optional sign followed by at least one digit.
    bool isWholeNumber(const std::string &text)
    {
        std::size_t position = 0;
        if (!text.empty() && (text.front() == '+' || text.front() == '-'))
        {
            position = 1;
        }

        if (position >= text.size())
        {
            return false;
        }

        for (; position < text.size(); ++position)
        {
            if (std::isdigit(static_cast<unsigned char>(text[position])) == 0)
            {
                return false;
            }
        }

        return true;
    }

    bool parseInteger(const std::map<std::string, std::string> &values,
                      const std::string &key,
                      long long &target,
                      std::string &errorMessage)
    {
        const std::string &raw = values.at(key);

        if (!isWholeNumber(raw))
        {
            errorMessage = "Value for '" + key + "' must be a whole number, got '" + raw + "'.";
            return false;
        }

        try
        {
            target = std::stoll(raw);
            return true;
        }
        catch (const std::exception &)
        {
            // Every accepted range ends at 2^32, so an int64 overflow is always
            // outside the allowed values.
            errorMessage = "Value for '" + key + "' is out of range, got '" + raw + "'.";
            return false;
        }
    }

    // Inclusive value range allowed for one config key.
    struct Range
    {
        long long minimum;
        long long maximum;
    };

    constexpr long long kMaxUnsigned32 = 4294967296LL; // 2^32, inclusive
    constexpr Range kNumCpuRange{1, 128};
    constexpr Range kQuantumCyclesRange{1, kMaxUnsigned32};
    constexpr Range kBatchProcessFreqRange{1, kMaxUnsigned32};
    constexpr Range kMinInsRange{1, kMaxUnsigned32};
    constexpr Range kMaxInsRange{1, kMaxUnsigned32};
    constexpr Range kDelayPerExecRange{0, kMaxUnsigned32};

    bool isWithin(long long value, Range range)
    {
        return value >= range.minimum && value <= range.maximum;
    }

    std::string rangeError(const std::string &key, long long value, Range range)
    {
        return "Value for '" + key + "' must be between " + std::to_string(range.minimum) + " and " +
               std::to_string(range.maximum) + ", got " + std::to_string(value) + ".";
    }

    std::string toLower(const std::string &text)
    {
        std::string lowered = text;
        for (char &character : lowered)
        {
            character = static_cast<char>(std::tolower(static_cast<unsigned char>(character)));
        }

        return lowered;
    }
}

bool loadConfig(const std::string &path, Config &config, std::string &errorMessage)
{
    std::ifstream input(path);
    if (!input.is_open())
    {
        errorMessage = "Could not open '" + path + "'.";
        return false;
    }

    std::map<std::string, std::string> values;

    std::string line;
    int lineNumber = 0;
    while (std::getline(input, line))
    {
        ++lineNumber;

        const std::string trimmed = trim(line);
        if (trimmed.empty() || trimmed.front() == '#')
        {
            continue;
        }

        const std::size_t separator = trimmed.find_first_of(" \t");
        if (separator == std::string::npos)
        {
            errorMessage = "Line " + std::to_string(lineNumber) + " of '" + path + "' has no value.";
            return false;
        }

        const std::string key = trimmed.substr(0, separator);
        const std::string value = stripQuotes(trim(trimmed.substr(separator + 1)));
        if (value.empty())
        {
            errorMessage = "Line " + std::to_string(lineNumber) + " of '" + path + "' has an empty value.";
            return false;
        }

        values[key] = value;
    }

    for (const char *key : {"num-cpu", "scheduler", "quantum-cycles", "batch-process-freq", "min-ins", "max-ins", "delay-per-exec"})
    {
        if (values.find(key) == values.end())
        {
            errorMessage = "Missing '" + std::string(key) + "' in '" + path + "'.";
            return false;
        }
    }

    Config parsed;
    parsed.scheduler = toLower(values.at("scheduler"));

    if (!parseInteger(values, "num-cpu", parsed.numCpu, errorMessage) ||
        !parseInteger(values, "quantum-cycles", parsed.quantumCycles, errorMessage) ||
        !parseInteger(values, "batch-process-freq", parsed.batchProcessFreq, errorMessage) ||
        !parseInteger(values, "min-ins", parsed.minIns, errorMessage) ||
        !parseInteger(values, "max-ins", parsed.maxIns, errorMessage) ||
        !parseInteger(values, "delay-per-exec", parsed.delayPerExec, errorMessage))
    {
        return false;
    }

    if (!validateConfig(parsed, errorMessage))
    {
        return false;
    }

    config = parsed;
    return true;
}

bool validateConfig(const Config &config, std::string &errorMessage)
{
    if (!isWithin(config.numCpu, kNumCpuRange))
    {
        errorMessage = rangeError("num-cpu", config.numCpu, kNumCpuRange);
        return false;
    }

    if (config.scheduler != "fcfs" && config.scheduler != "rr")
    {
        errorMessage = "Value for 'scheduler' must be \"fcfs\" or \"rr\", got '" + config.scheduler + "'.";
        return false;
    }

    if (!isWithin(config.quantumCycles, kQuantumCyclesRange))
    {
        errorMessage = rangeError("quantum-cycles", config.quantumCycles, kQuantumCyclesRange);
        return false;
    }

    if (!isWithin(config.batchProcessFreq, kBatchProcessFreqRange))
    {
        errorMessage = rangeError("batch-process-freq", config.batchProcessFreq, kBatchProcessFreqRange);
        return false;
    }

    if (!isWithin(config.minIns, kMinInsRange))
    {
        errorMessage = rangeError("min-ins", config.minIns, kMinInsRange);
        return false;
    }

    if (!isWithin(config.maxIns, kMaxInsRange))
    {
        errorMessage = rangeError("max-ins", config.maxIns, kMaxInsRange);
        return false;
    }

    if (!isWithin(config.delayPerExec, kDelayPerExecRange))
    {
        errorMessage = rangeError("delay-per-exec", config.delayPerExec, kDelayPerExecRange);
        return false;
    }

    if (config.minIns > config.maxIns)
    {
        errorMessage = "'min-ins' (" + std::to_string(config.minIns) + ") must not exceed 'max-ins' (" +
                       std::to_string(config.maxIns) + ").";
        return false;
    }

    return true;
}

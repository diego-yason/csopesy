#include "commands/InitializeCommand.h"

#include <string>

#include "config/Config.h"

namespace
{
    constexpr const char *kDefaultConfigPath = "config.txt";

    std::string trim(const std::string &text)
    {
        const std::size_t first = text.find_first_not_of(" \t\r\n");
        if (first == std::string::npos)
        {
            return std::string();
        }

        const std::size_t last = text.find_last_not_of(" \t\r\n");
        return text.substr(first, last - first + 1);
    }
}

InitializeCommand::InitializeCommand(SessionState &session)
    : session_(session)
{
}

const char *InitializeCommand::name() const
{
    return "initialize";
}

const char *InitializeCommand::description() const
{
    return "reads config.txt and prepares the system";
}

std::vector<std::string> InitializeCommand::usage() const
{
    return {"initialize [config path]"};
}

void InitializeCommand::execute(CommandContext &context, const std::string &arguments)
{
    if (session_.initialized())
    {
        context.printLine("System is already initialized.");
        return;
    }

    const std::string trimmed = trim(arguments);
    const std::string path = trimmed.empty() ? kDefaultConfigPath : trimmed;

    Config config;
    std::string errorMessage;
    if (!loadConfig(path, config, errorMessage))
    {
        context.printLine(errorMessage);
        return;
    }

    session_.initialize(config);

    context.prepareOutput(8);
    context.printLine("Initialized system from '" + path + "':");
    context.printLine("  num-cpu: " + std::to_string(config.numCpu));
    context.printLine("  scheduler: " + config.scheduler);
    context.printLine("  quantum-cycles: " + std::to_string(config.quantumCycles));
    context.printLine("  batch-process-freq: " + std::to_string(config.batchProcessFreq));
    context.printLine("  min-ins: " + std::to_string(config.minIns));
    context.printLine("  max-ins: " + std::to_string(config.maxIns));
    context.printLine("  delay-per-exec: " + std::to_string(config.delayPerExec));
}

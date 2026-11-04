#include "commands/ReportUtilCommand.h"

#include <fstream>
#include <string>
#include <vector>

#include "commands/ScreenReport.h"

namespace
{
    constexpr const char *kReportFile = "csopesy-log.txt";
}

ReportUtilCommand::ReportUtilCommand(const Scheduler &scheduler)
    : scheduler_(scheduler)
{
}

const char *ReportUtilCommand::name() const
{
    return "report-util";
}

const char *ReportUtilCommand::description() const
{
    return "prints the full process report and appends it to csopesy-log.txt";
}

bool ReportUtilCommand::requiresInitialization() const
{
    return true;
}

void ReportUtilCommand::execute(CommandContext &context, const std::string &arguments)
{
    (void)arguments;

    // Same report as screen -ls, but never truncated and including the
    // processes still queued for a core and the ones parked by SLEEP.
    ReportOptions options;
    options.truncate = false;
    options.includeQueued = true;
    options.includeSleeping = true;
    const std::vector<std::string> lines = buildScreenReport(scheduler_, options);

    context.prepareOutput(static_cast<int>(lines.size()));
    for (const std::string &line : lines)
    {
        context.printLine(line);
    }

    std::ofstream file(kReportFile); // overwrite the previous report
    if (!file.is_open())
    {
        context.printLine(std::string("Could not write '") + kReportFile + "'.");
        return;
    }

    for (const std::string &line : lines)
    {
        file << line << '\n';
    }
}

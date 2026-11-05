#include "commands/ScreenReport.h"

#include <cstddef>
#include <string>
#include <vector>

#include "console/TimeFormat.h"
#include "process/Process.h"

namespace
{
    // Truncated listings keep this many leading and trailing entries.
    constexpr std::size_t kHeadEntries = 2;
    constexpr std::size_t kTailEntries = 1;

    // screen -ls overrides the console's available-line count with its own
    // budget, so the layout and truncation do not depend on the window size.
    constexpr int kListAvailableLines = 20;

    // What the report always contains regardless of entries: three stat lines,
    // a blank, the opening and closing rules, one header per section, and one
    // blank between sections.
    constexpr std::size_t kReportFixedLines = 6;

    // Appends entries, keeping the head, an ellipsis, and the tail when needed.
    void appendEntries(std::vector<std::string> &lines, const std::vector<std::string> &entries, bool truncate)
    {
        const std::size_t truncatedLines = kHeadEntries + kTailEntries + 1; // includes the ellipsis

        if (!truncate || entries.size() <= truncatedLines)
        {
            lines.insert(lines.end(), entries.begin(), entries.end());
            return;
        }

        for (std::size_t index = 0; index < kHeadEntries; ++index)
        {
            lines.push_back(entries[index]);
        }

        lines.push_back("...");

        for (std::size_t index = entries.size() - kTailEntries; index < entries.size(); ++index)
        {
            lines.push_back(entries[index]);
        }
    }
}

std::vector<std::string> buildScreenReport(const Scheduler &scheduler, const ReportOptions &options)
{
    const std::vector<Scheduler::ProcessSummary> processes = scheduler.processSummaries();

    std::vector<std::string> running;
    std::vector<std::string> queued;
    std::vector<std::string> sleeping;
    std::vector<std::string> finished;

    for (const Scheduler::ProcessSummary &process : processes)
    {
        const std::string progress = std::to_string(process.currentLine) + " / " + std::to_string(process.totalLines);

        if (process.state == ProcessState::Finished)
        {
            finished.push_back(process.name + " (" + formatTimestamp(process.finishedAt) + ") Finished " + progress);
        }
        else if (process.core >= 0)
        {
            // On a core, including the cycle a preempted process is swapped out.
            running.push_back(process.name + " (" + formatTimestamp(process.attachedAt) + ") Core: " +
                              std::to_string(process.core) + "  " + progress);
        }
        else if (process.state == ProcessState::Waiting)
        {
            // Parked by SLEEP: show the wait that is still to go.
            sleeping.push_back(process.name + " (" +
                               (process.detached ? formatTimestamp(process.detachedAt) : std::string("null")) +
                               ") Sleeping " + progress + "  (cycles remaining: " +
                               std::to_string(process.sleepCycles) + ")");
        }
        else
        {
            // Queued processes show when they last lost their core; one that has
            // never run has no detach time at all, reported as null.
            queued.push_back(process.name + " (" +
                             (process.detached ? formatTimestamp(process.detachedAt) : std::string("null")) +
                             ") Queued " + progress);
        }
    }

    // "Used" is the allocation the cycle started with, recorded before anything
    // ran. The process list below is a live snapshot, so a process that slept
    // or finished during that cycle has already left its core while the core
    // still counts as used for the cycle.
    const std::size_t busy = scheduler.busyCores();
    const std::size_t cores = scheduler.coreCount();
    const std::size_t available = cores > busy ? cores - busy : 0;

    // Utilization is the share of cores the cycle ran on, as a whole percentage.
    const std::size_t utilization = cores > 0 ? (busy * 100) / cores : 0;

    // Entries get whatever the fixed block leaves of this command's budget.
    const std::size_t sections = 2 + (options.includeQueued ? 1u : 0u) + (options.includeSleeping ? 1u : 0u);
    const std::size_t fixed = kReportFixedLines + sections + (sections - 1);
    const std::size_t budget = kListAvailableLines > static_cast<int>(fixed)
                                   ? static_cast<std::size_t>(kListAvailableLines) - fixed
                                   : 0;

    std::size_t entries = running.size() + finished.size();
    if (options.includeQueued)
    {
        entries += queued.size();
    }
    if (options.includeSleeping)
    {
        entries += sleeping.size();
    }
    const bool cut = options.truncate && entries > budget;

    std::vector<std::string> lines;
    lines.push_back("CPU utilization: " + std::to_string(utilization) + "%");
    lines.push_back("Cores used: " + std::to_string(busy));
    lines.push_back("Cores available: " + std::to_string(available));
    lines.push_back("");
    lines.push_back("---");
    lines.push_back("Running processes:");
    appendEntries(lines, running, cut);
    lines.push_back("");

    if (options.includeQueued)
    {
        lines.push_back("Queued processes:");
        appendEntries(lines, queued, cut);
        lines.push_back("");
    }

    if (options.includeSleeping)
    {
        lines.push_back("Sleeping processes:");
        appendEntries(lines, sleeping, cut);
        lines.push_back("");
    }

    lines.push_back("Finished processes:");
    appendEntries(lines, finished, cut);
    lines.push_back("---");

    return lines;
}

#pragma once

#include <string>
#include <vector>

#include "scheduler/Scheduler.h"

// How much of the report to build.
struct ReportOptions
{
    // Keep the head, an ellipsis, and the tail for sections that do not fit the
    // command's line budget. `screen -ls` truncates; report-util does not.
    bool truncate = false;

    // Add the section for processes waiting in the ready queue.
    bool includeQueued = false;

    // Add the section for processes parked by SLEEP.
    bool includeSleeping = false;
};

// Builds the `screen -ls` style report: CPU stats, the processes holding a
// core, and the finished ones. Each optional section sits between the running
// and finished ones: queued processes show when they last left a core (or a
// null timestamp if they never ran), sleeping ones show how many cycles they
// still have to wait.
std::vector<std::string> buildScreenReport(const Scheduler &scheduler, const ReportOptions &options);

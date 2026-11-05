#pragma once

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

#include "config/SessionState.h"
#include "cpu/CpuClock.h"
#include "process/InstructionGenerator.h"
#include "process/Process.h"
#include "scheduler/SchedulingPolicy.h"

// Keeps time for the simulated machine. Every CPU cycle it wakes sleepers,
// generates one batch of processes while the scheduler is running, and lets
// each logical core retire one instruction. All of it runs on the clock
// thread, so cores are plain state rather than OS threads.
class Scheduler
{
public:
    // One logical CPU. `process` is null while the core is idle.
    struct CpuCore
    {
        int id = 0;
        Process *process = nullptr;
        std::size_t quantumUsed = 0;

        // Set when the running process used up its quantum. The core keeps the
        // process until the next cycle, when it is swapped for queued work.
        bool needsSwap = false;
    };

    Scheduler(CpuClock &clock, const SessionState &session);

    // Same, with a fixed seed so generated programs are reproducible.
    Scheduler(CpuClock &clock, const SessionState &session, unsigned int seed);

    ~Scheduler();

    Scheduler(const Scheduler &) = delete;
    Scheduler &operator=(const Scheduler &) = delete;

    // Starts generating processes and makes sure the CPU clock is ticking.
    void start();

    // Starts the CPU clock so ready processes get scheduled, without enabling
    // batch process generation. Used by screen -s.
    void startCpu();

    // True while the CPU clock is running.
    bool cpuRunning() const;

    // Stops generating processes. Already-created processes keep running.
    void stop();

    bool running() const;

    // Receives every line a running process prints, on the clock thread. Used
    // to mirror a process's output into its attached screen as it happens.
    using OutputListener = std::function<void(const Process &process, int core, const std::string &line)>;

    void setOutputListener(OutputListener listener);

    // Advances the machine by one cycle. CpuClock calls this; tests may call it
    // directly to step time deterministically.
    void tick();

    // How many processes have been created so far.
    std::size_t processCount() const;

    // How many processes have finished.
    std::size_t finishedCount() const;

    // How many cores were holding a process when the last cycle began, right
    // after dispatch. Sampling between ticks then reports the allocation the
    // cycle ran with instead of the slots a SLEEP or a finish frees later on.
    std::size_t busyCores() const;

    // Number of cores the machine has.
    std::size_t coreCount() const;

    // Lightweight copy of a process for listings.
    struct ProcessSummary
    {
        int pid = -1;
        std::string name;
        ProcessState state = ProcessState::Ready;
        int core = -1;
        std::size_t currentLine = 0;
        std::size_t totalLines = 0;
        std::chrono::system_clock::time_point attachedAt;
        std::chrono::system_clock::time_point finishedAt;

        // Last time the process left a core; epoch means it never ran.
        std::chrono::system_clock::time_point detachedAt;
        bool detached = false;

        // Cycles a sleeping process still has to wait out; 0 unless Waiting.
        std::uint8_t sleepCycles = 0;
    };

    // Copy of a process including its output log.
    struct ProcessDetails
    {
        int pid = -1;
        std::string name;
        ProcessState state = ProcessState::Ready;
        std::size_t currentLine = 0;
        std::size_t totalLines = 0;
        std::vector<ProcessLogEntry> log;
    };

    // Copies every process in creation order.
    std::vector<ProcessSummary> processSummaries() const;

    // Copies the process with this name; false when it does not exist.
    bool processDetails(const std::string &name, ProcessDetails &details) const;

    // Copies the state of the process with this name. False when it does not
    // exist.
    bool processState(const std::string &name, ProcessState &state) const;

    // Creates one process immediately with `name` and the generated program.
    // Names are first come first served: returns null when the session is not
    // initialized or an earlier process already owns that name. Auto-generated
    // processes fall back to a "-a" suffix instead of being refused.
    Process *createNamedProcess(const std::string &name);

private:
    void onCycle(std::uint64_t cycle);
    void wakeSleepingProcesses();
    void dispatchCores();

    // Builds the cores, policy, and delays from the loaded config. Called from
    // start() and from createNamedProcess(), so a process can be created before
    // the clock is running.
    void ensureMachineLocked();

    // `name` may be empty, in which case "p-<pid>" is used.
    Process *createProcessLocked(const std::string &name);

    CpuClock &clock_;
    const SessionState &session_;
    InstructionGenerator generator_;
    std::unique_ptr<SchedulingPolicy> policy_;

    mutable std::mutex mutex_;
    OutputListener output_listener_;
    bool running_ = false;
    std::uint64_t elapsedCycles_ = 0;
    std::uint64_t delay_per_exec_ = 0;
    int nextPid_ = 0;

    // Cores holding a process when the last cycle began.
    std::size_t cycle_busy_ = 0;

    std::vector<std::unique_ptr<Process>> processes_;
    std::deque<Process *> ready_;
    std::vector<CpuCore> cores_;
};

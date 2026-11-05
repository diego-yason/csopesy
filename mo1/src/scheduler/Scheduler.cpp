#include "scheduler/Scheduler.h"

#include <chrono>
#include <random>
#include <string>
#include <utility>

#include "process/Interpreter.h"
#include "process/ProcessOutput.h"

Scheduler::Scheduler(CpuClock &clock, const SessionState &session)
    : Scheduler(clock, session, std::random_device{}())
{
}

Scheduler::Scheduler(CpuClock &clock, const SessionState &session, unsigned int seed)
    : clock_(clock),
      session_(session),
      generator_(seed)
{
    clock_.setCycleHandler([this](std::uint64_t cycle)
                           { onCycle(cycle); });
}

Scheduler::~Scheduler()
{
    stop();

    // Join the clock thread before dropping the handler so no cycle callback
    // can run against a destroyed scheduler.
    clock_.stop();
    clock_.setCycleHandler({});
}

void Scheduler::start()
{
    std::lock_guard<std::mutex> lock(mutex_);

    if (running_)
    {
        return;
    }

    ensureMachineLocked();

    running_ = true;
    elapsedCycles_ = 0;
    cycle_busy_ = 0;
    clock_.start();
}

void Scheduler::startCpu()
{
    std::lock_guard<std::mutex> lock(mutex_);
    ensureMachineLocked();
    clock_.start();
}

bool Scheduler::cpuRunning() const
{
    std::lock_guard<std::mutex> lock(mutex_);
    return clock_.running();
}

void Scheduler::stop()
{
    std::lock_guard<std::mutex> lock(mutex_);
    running_ = false;
}

bool Scheduler::running() const
{
    std::lock_guard<std::mutex> lock(mutex_);
    return running_;
}

void Scheduler::setOutputListener(OutputListener listener)
{
    std::lock_guard<std::mutex> lock(mutex_);
    output_listener_ = std::move(listener);
}

std::size_t Scheduler::processCount() const
{
    std::lock_guard<std::mutex> lock(mutex_);
    return processes_.size();
}

std::size_t Scheduler::finishedCount() const
{
    std::lock_guard<std::mutex> lock(mutex_);

    std::size_t count = 0;
    for (const auto &process : processes_)
    {
        if (process->state == ProcessState::Finished)
        {
            ++count;
        }
    }

    return count;
}

std::size_t Scheduler::busyCores() const
{
    std::lock_guard<std::mutex> lock(mutex_);

    // The allocation this cycle started with, not the live slots: a process
    // that slept or finished during the cycle has already given up its core,
    // but the cycle still ran on it.
    return cycle_busy_;
}

std::size_t Scheduler::coreCount() const
{
    std::lock_guard<std::mutex> lock(mutex_);

    if (!cores_.empty())
    {
        return cores_.size();
    }

    const long long configured = session_.config().numCpu;
    return configured > 0 ? static_cast<std::size_t>(configured) : 0;
}

std::vector<Scheduler::ProcessSummary> Scheduler::processSummaries() const
{
    std::lock_guard<std::mutex> lock(mutex_);

    std::vector<ProcessSummary> summaries;
    summaries.reserve(processes_.size());
    for (const auto &process : processes_)
    {
        ProcessSummary summary;
        summary.pid = process->pid;
        summary.name = process->name;
        summary.state = process->state;
        summary.currentLine = process->currentLine();
        summary.totalLines = process->totalLines();
        summary.attachedAt = process->attachedAt;
        summary.finishedAt = process->finishedAt;
        summary.detachedAt = process->detachedAt;
        summary.detached = process->hasDetached();
        summary.sleepCycles = process->sleepCycles;

        for (const CpuCore &core : cores_)
        {
            if (core.process == process.get())
            {
                summary.core = core.id;
                break;
            }
        }

        summaries.push_back(std::move(summary));
    }

    return summaries;
}

bool Scheduler::processDetails(const std::string &name, ProcessDetails &details) const
{
    std::lock_guard<std::mutex> lock(mutex_);

    for (const auto &process : processes_)
    {
        if (process->name == name)
        {
            details.pid = process->pid;
            details.name = process->name;
            details.state = process->state;
            details.currentLine = process->currentLine();
            details.totalLines = process->totalLines();
            details.log = process->log;
            return true;
        }
    }

    return false;
}

bool Scheduler::processState(const std::string &name, ProcessState &state) const
{
    std::lock_guard<std::mutex> lock(mutex_);

    for (const auto &process : processes_)
    {
        if (process->name == name)
        {
            state = process->state;
            return true;
        }
    }

    return false;
}

namespace
{
    bool nameTaken(const std::vector<std::unique_ptr<Process>> &processes, const std::string &name)
    {
        for (const auto &process : processes)
        {
            if (process->name == name)
            {
                return true;
            }
        }

        return false;
    }

    // Records what a retiring instruction prints in the process log, tagged
    // with the core that produced it.
    class ProcessLogOutput : public ProcessOutput
    {
    public:
        ProcessLogOutput(Process &process, int coreId, const Scheduler::OutputListener *listener)
            : process_(process),
              core_id_(coreId),
              listener_(listener)
        {
        }

        void writeLine(const std::string &text) override
        {
            if (process_.log.size() >= Process::kMaxLogEntries)
            {
                process_.log.erase(process_.log.begin());
            }

            process_.log.push_back(ProcessLogEntry{std::chrono::system_clock::now(), core_id_, text});

            // Mirror the line to the screen that is showing this process, after
            // logging it, so the console and process-smi agree.
            if (listener_ != nullptr && *listener_)
            {
                (*listener_)(process_, core_id_, text);
            }
        }

    private:
        Process &process_;
        int core_id_;
        const Scheduler::OutputListener *listener_ = nullptr;
    };
}

void Scheduler::ensureMachineLocked()
{
    if (cores_.empty())
    {
        const long long cpuCount = session_.config().numCpu;
        const int count = cpuCount > 0 ? static_cast<int>(cpuCount) : 1;

        for (int id = 0; id < count; ++id)
        {
            CpuCore core;
            core.id = id;
            cores_.push_back(core);
        }
    }

    if (policy_ == nullptr)
    {
        const Config &config = session_.config();
        const std::size_t quantum = config.quantumCycles > 0 ? static_cast<std::size_t>(config.quantumCycles) : 1;
        policy_ = makeSchedulingPolicy(config.scheduler, quantum);
    }

    const Config &config = session_.config();
    delay_per_exec_ = config.delayPerExec > 0 ? static_cast<std::uint64_t>(config.delayPerExec) : 0;
}

Process *Scheduler::createNamedProcess(const std::string &name)
{
    std::lock_guard<std::mutex> lock(mutex_);

    if (!session_.initialized() || name.empty())
    {
        return nullptr;
    }

    for (const auto &process : processes_)
    {
        if (process->name == name)
        {
            return nullptr;
        }
    }

    ensureMachineLocked();
    return createProcessLocked(name);
}

void Scheduler::tick()
{
    std::lock_guard<std::mutex> lock(mutex_);

    wakeSleepingProcesses();

    if (running_)
    {
        ++elapsedCycles_;

        const long long frequency = session_.config().batchProcessFreq;
        if (frequency > 0 && elapsedCycles_ % static_cast<std::uint64_t>(frequency) == 0)
        {
            createProcessLocked(std::string());
        }
    }

    dispatchCores();
}

void Scheduler::onCycle(std::uint64_t cycle)
{
    (void)cycle;
    tick();
}

void Scheduler::wakeSleepingProcesses()
{
    for (const auto &process : processes_)
    {
        // tickSleep() only acts on Waiting processes and flips them back to
        // Ready when their countdown reaches zero.
        if (process->tickSleep())
        {
            policy_->enqueue(ready_, process.get());
        }
    }
}

void Scheduler::dispatchCores()
{
    if (cores_.empty() || policy_ == nullptr)
    {
        return;
    }

    // Phase 1: release preempted cores, then hand queued work to idle ones.
    // Doing this before any core runs keeps a process preempted this cycle from
    // being picked up again by another core in the same cycle.
    cycle_busy_ = 0;
    for (CpuCore &core : cores_)
    {
        if (core.needsSwap)
        {
            core.process = nullptr;
            core.needsSwap = false;
        }

        if (core.process == nullptr)
        {
            core.process = policy_->dequeue(ready_);
            if (core.process != nullptr)
            {
                core.process->state = ProcessState::Running;
                core.process->attachedAt = std::chrono::system_clock::now();
                core.quantumUsed = 0;
            }
        }

        // Recorded here, before anything runs, so a snapshot taken between
        // ticks shows the load this cycle started with.
        if (core.process != nullptr)
        {
            ++cycle_busy_;
        }
    }

    // Phase 2: every busy core retires one instruction.
    for (CpuCore &core : cores_)
    {
        if (core.process == nullptr)
        {
            continue;
        }

        Process &process = *core.process;
        ++core.quantumUsed;

        if (process.delayRemaining > 0)
        {
            // Busy wait: the process keeps the core while its delay runs down.
            --process.delayRemaining;
        }
        else
        {
            ProcessLogOutput output(process, core.id, &output_listener_);
            const bool advanced = Interpreter::step(process, output);

            // The delay applies before the next instruction, so it is armed
            // again as soon as one retires.
            process.delayRemaining = delay_per_exec_;

            if (!advanced)
            {
                process.state = ProcessState::Finished;
                process.finishedAt = std::chrono::system_clock::now();
                core.process = nullptr;
                continue;
            }

            if (process.state == ProcessState::Waiting)
            {
                // SLEEP parked it; wakeSleepingProcesses() requeues it later.
                process.detachedAt = std::chrono::system_clock::now();
                core.process = nullptr;
                continue;
            }
        }

        if (policy_->shouldPreempt(core.quantumUsed, ready_))
        {
            process.state = ProcessState::Ready;
            process.detachedAt = std::chrono::system_clock::now();
            policy_->enqueue(ready_, &process);

            // The core keeps pointing at the process for the rest of this
            // cycle; the next Phase 1 replaces it. That keeps the core counts
            // truthful between ticks instead of showing an idle machine.
            core.needsSwap = true;
        }
    }
}

Process *Scheduler::createProcessLocked(const std::string &name)
{
    const Config &config = session_.config();

    auto process = std::make_unique<Process>();
    process->pid = nextPid_++;

    if (name.empty())
    {
        // Auto-generated names are p-<pid>; a name already claimed by an
        // earlier process gets -a appended until it is free.
        std::string candidate = "p-" + std::to_string(process->pid);
        while (nameTaken(processes_, candidate))
        {
            candidate += "-a";
        }

        process->name = candidate;
    }
    else
    {
        process->name = name;
    }

    process->state = ProcessState::Ready;
    process->delayRemaining = delay_per_exec_;

    // Every process starts with x = 0 in its data segment: the generated
    // PRINT("Value from: " + x) / ADD(x, x, n) program depends on it.
    process->data.variables["x"] = 0;

    process->text.instructions = generator_.generate(static_cast<std::size_t>(config.minIns),
                                                     static_cast<std::size_t>(config.maxIns));

    Process *created = process.get();
    processes_.push_back(std::move(process));
    policy_->enqueue(ready_, created);
    return created;
}

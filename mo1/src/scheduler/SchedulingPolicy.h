#pragma once

#include <cstddef>
#include <deque>
#include <memory>
#include <string>

class Process;

// Decides which ready process runs next and when a running one loses the core.
class SchedulingPolicy
{
public:
    virtual ~SchedulingPolicy() = default;

    // Called whenever a process becomes ready: created, woken from SLEEP, or
    // preempted.
    virtual void enqueue(std::deque<Process *> &ready, Process *process) const = 0;

    // Pops the next process to run, or returns null when nothing is waiting.
    virtual Process *dequeue(std::deque<Process *> &ready) const = 0;

    // True when a process that has used `quantumUsed` cycles must give up the
    // core. `ready` is the waiting queue, so a policy can avoid a pointless
    // switch when nothing else can run.
    virtual bool shouldPreempt(std::size_t quantumUsed, const std::deque<Process *> &ready) const = 0;
};

// First come first serve: one FIFO queue, and a process keeps the core until it
// finishes or blocks itself with SLEEP.
class FirstComeFirstServePolicy : public SchedulingPolicy
{
public:
    void enqueue(std::deque<Process *> &ready, Process *process) const override;
    Process *dequeue(std::deque<Process *> &ready) const override;
    bool shouldPreempt(std::size_t quantumUsed, const std::deque<Process *> &ready) const override;
};

// Round robin: FIFO queue, but a process loses the core after `quantum` cycles
// and goes to the back of the line.
class RoundRobinPolicy : public SchedulingPolicy
{
public:
    explicit RoundRobinPolicy(std::size_t quantum);

    void enqueue(std::deque<Process *> &ready, Process *process) const override;
    Process *dequeue(std::deque<Process *> &ready) const override;
    bool shouldPreempt(std::size_t quantumUsed, const std::deque<Process *> &ready) const override;

private:
    std::size_t quantum_;
};

// Builds the policy named in config.txt ("fcfs" or "rr").
std::unique_ptr<SchedulingPolicy> makeSchedulingPolicy(const std::string &name, std::size_t quantum);

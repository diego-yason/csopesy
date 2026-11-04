#include "scheduler/SchedulingPolicy.h"

void FirstComeFirstServePolicy::enqueue(std::deque<Process *> &ready, Process *process) const
{
    ready.push_back(process);
}

Process *FirstComeFirstServePolicy::dequeue(std::deque<Process *> &ready) const
{
    if (ready.empty())
    {
        return nullptr;
    }

    Process *process = ready.front();
    ready.pop_front();
    return process;
}

bool FirstComeFirstServePolicy::shouldPreempt(std::size_t quantumUsed, const std::deque<Process *> &ready) const
{
    // FCFS is non-preemptive: only SLEEP or completion releases the core.
    (void)quantumUsed;
    (void)ready;
    return false;
}

RoundRobinPolicy::RoundRobinPolicy(std::size_t quantum)
    : quantum_(quantum > 0 ? quantum : 1)
{
}

void RoundRobinPolicy::enqueue(std::deque<Process *> &ready, Process *process) const
{
    ready.push_back(process);
}

Process *RoundRobinPolicy::dequeue(std::deque<Process *> &ready) const
{
    if (ready.empty())
    {
        return nullptr;
    }

    Process *process = ready.front();
    ready.pop_front();
    return process;
}

bool RoundRobinPolicy::shouldPreempt(std::size_t quantumUsed, const std::deque<Process *> &ready) const
{
    // Round robin only switches when the quantum is spent and somebody else is
    // waiting; otherwise the core would sit idle for a cycle.
    if (ready.empty())
    {
        return false;
    }

    return quantumUsed >= quantum_;
}

std::unique_ptr<SchedulingPolicy> makeSchedulingPolicy(const std::string &name, std::size_t quantum)
{
    if (name == "rr")
    {
        return std::make_unique<RoundRobinPolicy>(quantum);
    }

    // "fcfs" and anything unrecognized fall back to first come first serve.
    return std::make_unique<FirstComeFirstServePolicy>();
}

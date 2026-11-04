#pragma once

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <map>
#include <string>
#include <vector>

#include "process/Instruction.h"

// Code of a process: one entry per instruction. "min-ins" and "max-ins" bound
// how many entries a generated process receives.
struct TextSegment
{
    std::vector<Instruction> instructions;

    std::size_t instructionCount() const
    {
        return instructions.size();
    }
};

// Globals and constants the program declares before it starts running. DECLARE
// stores its variables here, one uint16 per name.
struct DataSegment
{
    std::map<std::string, std::uint16_t> variables;

    std::size_t variableCount() const
    {
        return variables.size();
    }
};

// Variables the program allocates while it runs.
struct HeapSegment
{
    std::map<std::string, long long> allocations;

    // Upper bound in bytes; 0 means unbounded.
    std::size_t memoryLimit = 0;

    std::size_t memoryUsed() const
    {
        return allocations.size() * sizeof(long long);
    }
};

// Call frames. The simulation is high level, so a frame is just a label and
// the stack does not need real storage.
struct StackSegment
{
    std::vector<std::string> frames;

    std::size_t depth() const
    {
        return frames.size();
    }
};

// One line a process printed, tagged with when and on which core it happened.
// Kept so an attached screen can show the output.
struct ProcessLogEntry
{
    std::chrono::system_clock::time_point timestamp;
    int core = 0;
    std::string message;
};

// Where a process is in its lifecycle.
enum class ProcessState
{
    Ready,   // queued, waiting for a core
    Running, // executing on a core
    Waiting, // blocked (for example, sleeping)
    Finished // done executing
};

// A program instance on the simulated machine, laid out as the four segments
// above. Segments hold symbolic contents rather than a real address space.
struct Process
{
    int pid = -1;
    std::string name;
    ProcessState state = ProcessState::Ready;

    // Cursor into the text segment. position[0] selects a top-level
    // instruction; each further index selects an instruction inside the body
    // of the FOR at that position.
    std::vector<std::size_t> position;

    // Iterations still owed by each enclosing FOR, outermost first. Holds one
    // entry per body level, so position.size() - 1 entries.
    std::vector<std::size_t> remainingIterations;

    // Cycles left before a sleeping process becomes ready again.
    std::uint8_t sleepCycles = 0;

    // When the process was last put on a core, and when it finished.
    std::chrono::system_clock::time_point attachedAt;
    std::chrono::system_clock::time_point finishedAt;

    // When the process last left a core without finishing (preempted by the
    // scheduler, or parked by SLEEP). A default-constructed value means it has
    // never run, and reports show a null timestamp for it.
    std::chrono::system_clock::time_point detachedAt;

    // CPU cycles this process still has to busy-wait on its core before the
    // next instruction runs (config "delay-per-exec").
    std::uint64_t delayRemaining = 0;

    TextSegment text;
    DataSegment data;
    HeapSegment heap;
    StackSegment stack;

    // Everything this process has printed so far. Only the latest
    // kMaxLogEntries lines are kept.
    std::vector<ProcessLogEntry> log;

    static constexpr std::size_t kMaxLogEntries = 7;

    // True once the process has left a core at least once.
    bool hasDetached() const
    {
        return detachedAt != std::chrono::system_clock::time_point{};
    }

    // One cycle passes for a sleeping process. Returns true when it wakes up.
    bool tickSleep()
    {
        if (state != ProcessState::Waiting)
        {
            return false;
        }

        if (sleepCycles > 0)
        {
            --sleepCycles;
        }

        if (sleepCycles > 0)
        {
            return false;
        }

        state = ProcessState::Ready;
        return true;
    }

    // Instruction lines in the text segment, counting FOR bodies.
    std::size_t totalLines() const
    {
        return countLines(text.instructions);
    }

    // 1-based line of the next instruction, flattening FOR loops in pre-order.
    std::size_t currentLine() const
    {
        const std::size_t total = totalLines();

        std::size_t line = 1;
        const std::vector<Instruction> *list = &text.instructions;

        for (std::size_t depth = 0; depth < position.size(); ++depth)
        {
            const std::size_t index = position[depth];
            if (index >= list->size())
            {
                // The cursor ran past the end: report the final line.
                return total > 0 ? total : 1;
            }

            for (std::size_t before = 0; before < index; ++before)
            {
                line += 1 + countLines((*list)[before].body);
            }

            if (depth + 1 < position.size())
            {
                line += 1; // the FOR line itself
                list = &(*list)[index].body;
            }
        }

        return total > 0 && line > total ? total : line;
    }

private:
    static std::size_t countLines(const std::vector<Instruction> &list)
    {
        std::size_t total = 0;
        for (const Instruction &instruction : list)
        {
            total += 1;
            if (!instruction.body.empty())
            {
                total += countLines(instruction.body);
            }
        }

        return total;
    }
};

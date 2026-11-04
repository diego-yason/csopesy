#pragma once

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

    TextSegment text;
    DataSegment data;
    HeapSegment heap;
    StackSegment stack;

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
};

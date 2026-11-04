#pragma once

#include <chrono>
#include <cstdint>
#include <atomic>
#include <functional>
#include <thread>

// The pseudo-CPU's core loop: a background thread that advances a cycle counter
// once per simulated cycle. It is started by the scheduler and keeps ticking
// until shutdown, even when the scheduler itself is stopped.
class CpuClock
{
public:
    // Invoked once per cycle with the new cycle number (1, 2, 3, ...).
    using CycleHandler = std::function<void(std::uint64_t cycle)>;

    // Half a millisecond per cycle, which is roughly a thousand simulated
    // instructions a second. Plain millisecond sleeps used to round every cycle
    // up to the 15.6 ms system timer tick, so the machine felt very slow.
    static constexpr std::chrono::microseconds kDefaultCycle{500};

    explicit CpuClock(std::chrono::microseconds cycleDuration = kDefaultCycle);

    ~CpuClock();

    CpuClock(const CpuClock &) = delete;
    CpuClock &operator=(const CpuClock &) = delete;

    // Install this before start(); it is not safe to swap while running.
    void setCycleHandler(CycleHandler handler);

    // Length of one simulated cycle; 0 or less runs the loop flat out. Set it
    // before start().
    void setCycleDuration(std::chrono::microseconds cycleDuration);

    // Starts the loop at cycle 0. Calling it again while running does nothing.
    void start();

    // Stops and joins the loop; only used on shutdown.
    void stop();

    bool running() const;
    std::uint64_t cycleCount() const;

private:
    void run();

    std::chrono::microseconds cycle_duration_;
    std::atomic<bool> running_{false};
    std::atomic<std::uint64_t> cycle_count_{0};
    CycleHandler cycle_handler_;
    std::thread thread_;
};

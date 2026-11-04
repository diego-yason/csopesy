#include "cpu/CpuClock.h"

#include <utility>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

// Available on Windows 10 1803+; older SDK headers may not define it.
#ifndef CREATE_WAITABLE_TIMER_HIGH_RESOLUTION
#define CREATE_WAITABLE_TIMER_HIGH_RESOLUTION 0x00000002
#endif
#endif

namespace
{
    // Sleeps for the exact cycle duration. A plain sleep_for() rounds every wait
    // up to the system timer resolution (about 15.6 ms on Windows), so a 1 ms
    // cycle actually took 15.6 ms and the simulated machine crawled. A
    // high-resolution waitable timer does not have that granularity, so the
    // clock runs at the rate it was configured for.
    class CycleSleeper
    {
    public:
        CycleSleeper() = default;

        ~CycleSleeper()
        {
#ifdef _WIN32
            if (timer_ != nullptr)
            {
                CloseHandle(timer_);
            }
#endif
        }

        CycleSleeper(const CycleSleeper &) = delete;
        CycleSleeper &operator=(const CycleSleeper &) = delete;

        void wait(std::chrono::microseconds duration)
        {
#ifdef _WIN32
            if (timer_ == nullptr)
            {
                timer_ = CreateWaitableTimerExW(nullptr,
                                                nullptr,
                                                CREATE_WAITABLE_TIMER_HIGH_RESOLUTION,
                                                TIMER_ALL_ACCESS);
            }

            if (timer_ != nullptr)
            {
                // Relative due time, in 100 ns units, counted backwards.
                const long long units = std::chrono::duration_cast<std::chrono::nanoseconds>(duration).count() / 100;

                LARGE_INTEGER due;
                due.QuadPart = units > 0 ? -units : -1;

                if (SetWaitableTimer(timer_, &due, 0, nullptr, nullptr, FALSE) != 0)
                {
                    WaitForSingleObject(timer_, INFINITE);
                    return;
                }
            }
#endif
            // Fallback: no high-resolution timer available.
            std::this_thread::sleep_for(duration);
        }

    private:
#ifdef _WIN32
        HANDLE timer_ = nullptr;
#endif
    };
}

CpuClock::CpuClock(std::chrono::microseconds cycleDuration)
    : cycle_duration_(cycleDuration)
{
}

void CpuClock::setCycleDuration(std::chrono::microseconds cycleDuration)
{
    // Only meaningful before start(); the loop reads it every cycle, but the
    // clock rate should not change under a running simulation.
    cycle_duration_ = cycleDuration;
}

CpuClock::~CpuClock()
{
    stop();
}

void CpuClock::setCycleHandler(CycleHandler handler)
{
    cycle_handler_ = std::move(handler);
}

void CpuClock::start()
{
    if (running_.exchange(true))
    {
        return;
    }

    thread_ = std::thread(&CpuClock::run, this);
}

void CpuClock::stop()
{
    if (!running_.exchange(false))
    {
        return;
    }

    if (thread_.joinable())
    {
        thread_.join();
    }
}

bool CpuClock::running() const
{
    return running_.load();
}

std::uint64_t CpuClock::cycleCount() const
{
    return cycle_count_.load(std::memory_order_relaxed);
}

void CpuClock::run()
{
    CycleSleeper sleeper;

    while (running_.load())
    {
        const std::uint64_t cycle = cycle_count_.fetch_add(1, std::memory_order_relaxed) + 1;

        if (cycle_handler_)
        {
            cycle_handler_(cycle);
        }

        // A zero (or negative) duration means "as fast as the machine allows":
        // the loop still yields so it does not starve the console thread.
        if (cycle_duration_.count() <= 0)
        {
            std::this_thread::yield();
            continue;
        }

        // Wait exactly one cycle; the wait is precise instead of rounding up to
        // the system timer resolution.
        sleeper.wait(cycle_duration_);
    }
}

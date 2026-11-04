#pragma once

#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <string>

// "(2026-11-05 14:03:22)"
inline std::string formatTimestamp(std::chrono::system_clock::time_point point)
{
    const std::time_t seconds = std::chrono::system_clock::to_time_t(point);

    const std::tm *converted = std::localtime(&seconds);
    if (converted == nullptr)
    {
        return "unknown time";
    }

    const std::tm local = *converted;

    std::ostringstream stream;
    stream << std::put_time(&local, "%Y-%m-%d %H:%M:%S");
    return stream.str();
}

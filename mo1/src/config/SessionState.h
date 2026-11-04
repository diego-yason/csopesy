#pragma once

#include "config/Config.h"

// Tracks whether the simulator has been initialized. Commands that depend on
// config.txt stay unavailable until "initialize" flips this.
class SessionState
{
public:
    bool initialized() const
    {
        return initialized_;
    }

    const Config &config() const
    {
        return config_;
    }

    void initialize(const Config &config)
    {
        config_ = config;
        initialized_ = true;
    }

private:
    bool initialized_ = false;
    Config config_{};
};

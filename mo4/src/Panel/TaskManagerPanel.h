#pragma once

#include "Panel.h"

// Task manager panel: lists and controls the processes the simulator is running.
// Contents to be filled in.
class TaskManagerPanel : public Panel
{
public:
    TaskManagerPanel();

    // Opens at a size large enough to show the whole table. Overridden because
    // ImGui's auto-fit under-estimates table widths on the first frames.
    void draw() override;

    void render() override;
};

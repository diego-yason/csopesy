#pragma once

#include "Panel.h"

// Example panel: shows basic information about the application.
class AboutPanel : public Panel
{
public:
    AboutPanel();

    void render() override;
};

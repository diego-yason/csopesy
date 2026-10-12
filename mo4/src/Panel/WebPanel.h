#pragma once

#include "Panel.h"

// Web panel: a mock browser toolbar with a URL bar.
// Purely cosmetic for now: the controls do not react and there is no page view.
class WebPanel : public Panel
{
public:
    WebPanel();

    // Opens at a size wide enough for the toolbar row.
    void draw() override;

    void render() override;

private:
    char m_url[256] = "https://www.example.com";
};

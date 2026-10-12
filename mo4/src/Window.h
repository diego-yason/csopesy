#pragma once

#include "Component/Button.h"
#include "Component/Texture.h"
#include "Panel/Panel.h"
#include "Panel/BottomPanel.h"

#include <functional>
#include <memory>
#include <string>
#include <vector>

// Full-viewport application window: the canvas the MO4 UI is built on.
class Window
{
public:
    Window();

    // Draws the window contents. Call once per frame, between
    // ImGui::NewFrame() and ImGui::Render().
    void render();

    // True once the user has asked to quit (the PWR button).
    bool shouldClose() const { return m_should_close; }

private:
    // Adds a bottom bar button that toggles the panel identified by `key`.
    void addPanelButton(const std::string &key, const std::function<std::unique_ptr<Panel>()> &factory);

    // Closes the panel with `key` if it is open, otherwise creates one.
    void togglePanel(const std::string &key, const std::function<std::unique_ptr<Panel>()> &factory);

    // Pop-up panels, owned by the window.
    std::vector<std::unique_ptr<Panel>> m_panels;

    // Background image drawn behind the canvas contents.
    Texture m_background;

    // Downscaled copy of the background, used for the bottom bar's backdrop.
    Texture m_backgroundBlur;

    // Bottom bar, drawn on top of everything else. Always open.
    BottomPanel m_bottomPanel;

    // Red power button shown to the left of the clock.
    Button m_powerButton;
    bool m_should_close = false;
};

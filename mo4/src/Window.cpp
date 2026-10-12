#include "Window.h"

#include "Panel/AboutPanel.h"
#include "Panel/TaskManagerPanel.h"
#include "Panel/WebPanel.h"

#include "imgui.h"

#include <algorithm>
#include <ctime>

namespace
{
    // Formats the current local time as "HH:MM:SS AM/PM | DD MMM YYYY".
    void formatLocalTime(char *buffer, size_t size)
    {
        const std::time_t now = std::time(nullptr);
        std::tm local{};
        localtime_s(&local, &now);
        std::strftime(buffer, size, "%I:%M:%S %p | %d %b %Y", &local);
    }
}

Window::Window()
    : m_powerButton("PWR", [this]
                    { m_should_close = true; })
{
    // Bottom bar entries toggle panels, which the window owns.
    addPanelButton("ABT", []
                   { return std::make_unique<AboutPanel>(); });
    addPanelButton("WEB", []
                   { return std::make_unique<WebPanel>(); });
    addPanelButton("TSK MGR", []
                   { return std::make_unique<TaskManagerPanel>(); });

    // The background lives at "static/bg.png" at runtime (CMake copies it next
    // to the executable). Fall back to the source tree for other working dirs.
    for (const char *candidate : {"static/bg.png", "src/static/bg.png", "../src/static/bg.png"})
    {
        if (m_background.loadFromFile(candidate))
        {
            // A tiny copy of the same image; magnified by the bottom bar it blurs.
            m_backgroundBlur.loadFromFile(candidate, 0.08f);
            break;
        }
    }

    m_bottomPanel.setBackgroundTexture(m_backgroundBlur.id());
}

void Window::addPanelButton(const std::string &key, const std::function<std::unique_ptr<Panel>()> &factory)
{
    m_bottomPanel.addButton(key, [this, key, factory]
                            { togglePanel(key, factory); });
}

void Window::togglePanel(const std::string &key, const std::function<std::unique_ptr<Panel>()> &factory)
{
    const auto existing = std::find_if(m_panels.begin(), m_panels.end(),
                                       [&key](const std::unique_ptr<Panel> &panel)
                                       { return panel->key() == key; });

    if (existing != m_panels.end())
    {
        // Already open: close it. Closed panels are erased at the end of the frame.
        (*existing)->setOpen(false);
        return;
    }

    m_panels.push_back(factory());
}

void Window::render()
{
    // Use Pos/Size (WorkPos/WorkSize are never populated in this ImGui build).
    const ImGuiViewport *viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->Pos);
    ImGui::SetNextWindowSize(viewport->Size);

    const ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
                                   ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse |
                                   ImGuiWindowFlags_NoBringToFrontOnFocus |
                                   ImGuiWindowFlags_NoNavFocus | ImGuiWindowFlags_NoSavedSettings;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::Begin("Main", nullptr, flags);
    ImGui::PopStyleVar(2);

    // Fill the canvas with the background image, behind everything else.
    if (m_background.id() != 0)
    {
        const ImVec2 pos = ImGui::GetWindowPos();
        const ImVec2 size = ImGui::GetWindowSize();
        ImGui::GetWindowDrawList()->AddImage(static_cast<ImTextureID>(m_background.id()), pos,
                                             ImVec2(pos.x + size.x, pos.y + size.y));
    }

    // Top bar: a red power button, then the live clock to its right.
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.72f, 0.13f, 0.13f, 1.00f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.84f, 0.20f, 0.20f, 1.00f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.56f, 0.09f, 0.09f, 1.00f));
    m_powerButton.draw();
    ImGui::PopStyleColor(3);

    // Keep the clock vertically aligned with the button beside it.
    ImGui::SameLine();
    ImGui::AlignTextToFramePadding();

    // Live clock, e.g. "03:07:45 PM | 11 Oct 2026".
    char clock_text[64];
    formatLocalTime(clock_text, sizeof(clock_text));
    ImGui::TextUnformatted(clock_text);

    ImGui::End();

    // Panels are drawn on top of the main canvas.
    for (const std::unique_ptr<Panel> &panel : m_panels)
        panel->draw();

    // Drop panels the user has closed.
    m_panels.erase(std::remove_if(m_panels.begin(), m_panels.end(),
                                  [](const std::unique_ptr<Panel> &panel)
                                  { return !panel->isOpen(); }),
                   m_panels.end());

    // The bottom bar is always drawn last, so it stays on top.
    m_bottomPanel.draw();
}

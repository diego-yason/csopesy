#include "Panel.h"

#include "imgui.h"

Panel::Panel(std::string key, std::string title, bool open)
    : m_key(std::move(key)), m_title(std::move(title)), m_open(open)
{
}

void Panel::draw()
{
    if (!m_open)
        return;

    // ImGui::Begin returns false when the window is collapsed; the window
    // must still be closed with ImGui::End(), so keep the End() unconditional.
    if (ImGui::Begin(m_title.c_str(), &m_open))
        render();

    ImGui::End();
}

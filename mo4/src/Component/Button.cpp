#include "Button.h"

#include "imgui.h"

Button::Button(std::string label, Action onClick)
    : m_label(std::move(label)), m_onClick(std::move(onClick))
{
}

bool Button::draw(float width, float height)
{
    const bool clicked = ImGui::Button(m_label.c_str(), ImVec2(width, height));
    if (clicked && m_onClick)
        m_onClick();

    return clicked;
}

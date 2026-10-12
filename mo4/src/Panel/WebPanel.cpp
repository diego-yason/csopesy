#include "WebPanel.h"

#include "imgui.h"

#include <cfloat>

namespace
{
    // Minimum size that still fits the whole toolbar row plus the title bar.
    // Kept in ImGui style units, so it scales with the global UI scale.
    constexpr float kMinWidth = 420.0f;
    constexpr float kMinHeight = 120.0f;
}

WebPanel::WebPanel()
    : Panel("WEB", "Web Browser")
{
}

void WebPanel::draw()
{
    ImGui::SetNextWindowSize(ImVec2(680.0f, 120.0f), ImGuiCond_FirstUseEver);
    // Clamped every frame, so the panel cannot be shrunk below the toolbar.
    ImGui::SetNextWindowSizeConstraints(ImVec2(kMinWidth, kMinHeight),
                                        ImVec2(FLT_MAX, FLT_MAX));
    Panel::draw();
}

void WebPanel::render()
{
    // Mock toolbar: navigation controls followed by a URL bar that takes the
    // rest of the row. Nothing here is wired up.
    const ImGuiStyle &style = ImGui::GetStyle();
    const float go_width = ImGui::CalcTextSize("Go").x + style.FramePadding.x * 2.0f;

    ImGui::Button("<");
    ImGui::SameLine();
    ImGui::Button(">");
    ImGui::SameLine();
    ImGui::Button("Reload");
    ImGui::SameLine();
    ImGui::Button("Home");
    ImGui::SameLine();

    // Negative width = "fill the available space, leaving room for the Go button".
    ImGui::SetNextItemWidth(-(go_width + style.ItemSpacing.x));
    ImGui::InputText("##url", m_url, IM_ARRAYSIZE(m_url));
    ImGui::SameLine();
    ImGui::Button("Go");
}

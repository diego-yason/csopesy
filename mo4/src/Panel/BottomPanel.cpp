#include "BottomPanel.h"

#include "imgui.h"

#include <utility>

namespace
{
    constexpr float kBottomPanelHeight = 30.0f;
    constexpr float kContentPadding = 5.0f;
    constexpr float kButtonGap = 6.0f;
}

BottomPanel::BottomPanel()
    : Panel("bottom-panel", "Bottom Panel", true)
{
}

void BottomPanel::draw()
{
    // NB: use Pos/Size, not WorkPos/WorkSize. This ImGui build never populates
    // the work area (it stays 0,0), which would push the bar off-screen.
    const ImGuiViewport *viewport = ImGui::GetMainViewport();
    const ImVec2 viewport_pos = viewport->Pos;
    const ImVec2 viewport_size = viewport->Size;

    const float height = kBottomPanelHeight;
    const ImVec2 pos(viewport_pos.x, viewport_pos.y + viewport_size.y - height);
    const ImVec2 size(viewport_size.x, height);

    ImGui::SetNextWindowPos(pos);
    ImGui::SetNextWindowSize(size);

    // NB: do NOT set ImGuiWindowFlags_NoBringToFrontOnFocus here. In this ImGui
    // build that flag pushes a window to the BACK of the draw order, which would
    // hide the bar behind the fullscreen canvas. Only the canvas wants it.
    const ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
                                   ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse |
                                   ImGuiWindowFlags_NoNavFocus | ImGuiWindowFlags_NoSavedSettings |
                                   ImGuiWindowFlags_NoScrollbar;

    // We paint our own background, so keep ImGui's window background invisible.
    // The padding insets the content while the backdrop still reaches the edges
    // (it is drawn from absolute coordinates below).
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(kContentPadding, kContentPadding));
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));

    ImGui::Begin("##BottomPanel", nullptr, flags);
    ImGui::PopStyleColor();
    ImGui::PopStyleVar(3);

    ImDrawList *draw_list = ImGui::GetWindowDrawList();
    const ImVec2 bottom_right(pos.x + size.x, pos.y + size.y);

    // Frosted backdrop. The background image is redrawn here from a tiny
    // downscaled copy, so magnifying it blurs what sits behind the bar. The
    // image fills the whole window, so sample its bottom slice.
    if (m_background_id != 0)
    {
        const float v0 = viewport_size.y > 0.0f
                             ? (viewport_size.y - height) / viewport_size.y
                             : 0.0f;
        draw_list->AddImage(static_cast<ImTextureID>(m_background_id), pos, bottom_right,
                            ImVec2(0.0f, v0), ImVec2(1.0f, 1.0f));
        draw_list->AddRectFilled(pos, bottom_right, IM_COL32(12, 14, 18, 150));
    }
    else
    {
        draw_list->AddRectFilled(pos, bottom_right, IM_COL32(12, 14, 18, 230));
    }

    // Light, hard divider along the top edge (a solid 1px strip, no AA).
    draw_list->AddRectFilled(pos, ImVec2(bottom_right.x, pos.y + 1.0f),
                             IM_COL32(255, 255, 255, 100));

    render();

    ImGui::End();
}

void BottomPanel::addButton(std::string label, Button::Action onClick)
{
    m_buttons.emplace_back(std::move(label), std::move(onClick));
}

void BottomPanel::render()
{
    // Horizontal flexbox: items share one row, left to right, separated by a
    // fixed gap.
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing,
                        ImVec2(kButtonGap, ImGui::GetStyle().ItemSpacing.y));

    bool first = true;
    for (Button &button : m_buttons)
    {
        if (!first)
            ImGui::SameLine();
        button.draw();
        first = false;
    }

    ImGui::PopStyleVar();
}

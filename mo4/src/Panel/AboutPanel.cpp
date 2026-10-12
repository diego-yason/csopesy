#include "AboutPanel.h"

#include "imgui.h"

AboutPanel::AboutPanel()
    : Panel("ABT", "About")
{
}

void AboutPanel::render()
{
    ImGui::Text("CSOPESY MO4");
    ImGui::Separator();
    ImGui::TextWrapped("A simple CPU scheduling simulator UI built with Dear ImGui, GLFW, and OpenGL.");
    ImGui::Spacing();
    ImGui::Text("Dear ImGui %s", IMGUI_VERSION);
}

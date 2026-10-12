#include "TaskManagerPanel.h"

#include "imgui.h"

namespace
{
    struct TaskRow
    {
        const char *process;
        const char *cpu;
        const char *memory;
        const char *disk;
        const char *network;
    };

    // Placeholder data until the simulator drives this panel.
    constexpr TaskRow kTasks[] = {
        {"chrome.exe", "12.4%", "1.28 GB", "3.2 MB/s", "845 KB/s"},
        {"code.exe", "6.8%", "742 MB", "1.1 MB/s", "96 KB/s"},
        {"csopesy.exe", "4.1%", "212 MB", "0.6 MB/s", "12 KB/s"},
        {"explorer.exe", "0.9%", "88 MB", "0.2 MB/s", "3 KB/s"},
        {"dwm.exe", "1.7%", "146 MB", "0.4 MB/s", "1 KB/s"},
    };
}

TaskManagerPanel::TaskManagerPanel()
    : Panel("TSK MGR", "Task Manager")
{
}

void TaskManagerPanel::draw()
{
    ImGui::SetNextWindowSize(ImVec2(720.0f, 240.0f), ImGuiCond_FirstUseEver);
    Panel::draw();
}

void TaskManagerPanel::render()
{
    ImGui::SeparatorText("Processes");

    // Stretch columns so they shrink with the panel, which lets the cell text
    // wrap (TextWrapped wraps at the column's right edge) instead of clipping.
    constexpr ImGuiTableFlags flags = ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg |
                                      ImGuiTableFlags_SizingStretchProp;

    if (!ImGui::BeginTable("##tasks", 5, flags))
        return;

    ImGui::TableSetupColumn("Process", ImGuiTableColumnFlags_WidthStretch, 1.6f);
    ImGui::TableSetupColumn("CPU", ImGuiTableColumnFlags_WidthStretch, 1.0f);
    ImGui::TableSetupColumn("Memory", ImGuiTableColumnFlags_WidthStretch, 1.2f);
    ImGui::TableSetupColumn("Disk", ImGuiTableColumnFlags_WidthStretch, 1.2f);
    ImGui::TableSetupColumn("Network", ImGuiTableColumnFlags_WidthStretch, 1.2f);
    ImGui::TableHeadersRow();

    for (const TaskRow &task : kTasks)
    {
        ImGui::TableNextRow();

        ImGui::TableSetColumnIndex(0);
        ImGui::TextWrapped("%s", task.process);
        ImGui::TableSetColumnIndex(1);
        ImGui::TextWrapped("%s", task.cpu);
        ImGui::TableSetColumnIndex(2);
        ImGui::TextWrapped("%s", task.memory);
        ImGui::TableSetColumnIndex(3);
        ImGui::TextWrapped("%s", task.disk);
        ImGui::TableSetColumnIndex(4);
        ImGui::TextWrapped("%s", task.network);
    }

    ImGui::EndTable();
}

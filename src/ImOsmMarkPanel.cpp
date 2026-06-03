#include "ImOsmMarkPanel.h"
#include "ImOsmMarkRenderer.h"
#include <imgui.h>

using namespace ImOsm::Rich;

void ImOsm::Rich::ShowMarkPanel(std::shared_ptr<RichMapPlot> plot) {
  ImGui::Begin("Mark Panel");

  static bool inited = false;
  if (!inited) {
    if (plot) InitMarkRenderer(plot);
    AddMarksFromApi();
    inited = true;
  }

  // if (ImGui::Button("Init Mark Renderer")) {
  //   if (plot) InitMarkRenderer(plot);
  // }
  //ImGui::SameLine();
  // if (ImGui::Button("Load Marks From API")) {
  //   AddMarksFromApi();
  // }
  // ImGui::SameLine();
  // if (ImGui::Button("Clear Marks")) {
  //   ClearMarks();
  // }

  ImGui::Separator();

  static float lat = 0.0f;
  static float lon = 0.0f;
  static char title[128] = "";

  ImGui::InputFloat("Lat", &lat, 0.0f, 0.0f, "%.6f");
  ImGui::InputFloat("Lon", &lon, 0.0f, 0.0f, "%.6f");
  ImGui::InputText("Title", title, sizeof(title));
  if (ImGui::Button("Add Mark")) {
    AddMarkGeo(lat, lon, std::string(title));
  }

  ImGui::End();
}

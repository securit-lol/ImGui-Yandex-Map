#include "ImOsmMarkRenderer.h"
#include <imosm_rich.h>
#include "YandexData.h"

using namespace ImOsm;
using namespace ImOsm::Rich;

namespace {
std::shared_ptr<MarkStorage> s_storage;
std::unique_ptr<MarkEditorWidget> s_editor;
std::shared_ptr<RichMapPlot> s_plot;
}

void ImOsm::Rich::InitMarkRenderer(std::shared_ptr<RichMapPlot> plot) {
  s_plot = plot;
  s_storage = std::make_shared<MarkStorage>();
  s_editor = std::make_unique<MarkEditorWidget>(s_plot, s_storage);
}

void ImOsm::Rich::AddMarkGeo(float lat, float lon, const std::string &title) {
  if (!s_editor) InitMarkRenderer(s_plot);
  if (s_editor) {
    s_editor->AddMarkCustom({lat, lon}, title);
  }
}

void ImOsm::Rich::AddMarksFromApi() {
  if (!s_editor) InitMarkRenderer(s_plot);
  api::GetAllStations();
  for (const auto &country : api::data::all_country_data) {
    for (const auto &region : country.regions) {
      for (const auto &lement : region.settlements) {
        if (s_editor)
          s_editor->AddMarkCustom({lement.latitude, lement.longitude},
                                  lement.title);
      }
    }
  }
}

void ImOsm::Rich::ClearMarks() {
  s_editor.reset();
  s_storage.reset();
}

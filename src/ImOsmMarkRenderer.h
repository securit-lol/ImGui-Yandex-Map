#pragma once
#include <memory>
#include <string>
#include <imosm_rich.h>

namespace ImOsm {
namespace Rich {
void InitMarkRenderer(std::shared_ptr<RichMapPlot> plot);
void AddMarkGeo(float lat, float lon, const std::string &title);
void AddMarksFromApi();
void ClearMarks();
} // namespace Rich
} // namespace ImOsm

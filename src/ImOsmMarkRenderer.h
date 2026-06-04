#pragma once
#include <memory>
#include <string>
#include <imosm_rich.h>

namespace ImOsm {
namespace Rich {
void InitMarkRenderer(std::shared_ptr<RichMapPlot> plot);
void AddMarksFromApi();
void SkanMarks();
void ClearMarks();
} // namespace Rich
} // namespace ImOsm

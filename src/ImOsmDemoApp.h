#pragma once
#include <imapp.h>
#include <imosm.h>
#include <imwrap.h>

namespace ImOsm {
namespace Rich {
class RichMapPlot;
} // namespace Rich
} // namespace ImOsm

class ImOsmDemoApp : public ImApp::MainWindow {
public:
  ImOsmDemoApp();
  ~ImOsmDemoApp();

protected:
  void beforeLoop() override;
  void firstPaint() override;
  void paint() override;

private:
  std::shared_ptr<ImOsm::MapPlot> _mapPlot;
};

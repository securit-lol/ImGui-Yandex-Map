#pragma once
#include <imapp.h>
#include <imosm.h>
#include <imosm_rich.h>
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
  std::shared_ptr<ImOsm::Rich::RichMapPlot> _mapPlot;
  
  std::unique_ptr<ImOsm::TileSourceWidget> _tileSourceWidget;
  std::unique_ptr<ImOsm::TileGrabberWidget> _tileGrabberWidget;

  const std::string _iniFileNameMain{"imosm_demo.ini"};
  const std::string _iniFileNameMark{"imosm_mark.ini"};
};

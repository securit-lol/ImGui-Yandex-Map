#include "ImOsmDemoApp.h"
#include <imgui_internal.h>
#include "ImOsmMarkRenderer.h"
#include "ImOsmMarkPanel.h"

using namespace ImOsm;
using namespace ImOsm::Rich;

ImOsmDemoApp::ImOsmDemoApp()
    : ImApp::MainWindow("ImOsm Demo Application"),
      _mapPlot{std::make_shared<RichMapPlot>()},
      _tileSourceWidget{std::make_unique<TileSourceWidget>(_mapPlot)},
      _tileGrabberWidget{std::make_unique<TileGrabberWidget>(_mapPlot)} {
  {
    mINI::INIStructure ini;
    mINI::INIFile iniFile(_iniFileNameMain);
    iniFile.read(ini);
    _mapPlot->loadState(ini);
    _tileSourceWidget->loadState(ini);
    _tileGrabberWidget->loadState(ini);

    _mapPlot->setBoundsGeo(ImOsm::MinLat, ImOsm::MaxLat, ImOsm::MinLon,
                           ImOsm::MaxLon);


    InitMarkRenderer(_mapPlot);
    AddMarksFromApi();          
                           
  }
}

ImOsmDemoApp::~ImOsmDemoApp() {
  {
    mINI::INIStructure ini;
    mINI::INIFile iniFile(_iniFileNameMain);
    _mapPlot->saveState(ini);
    _tileSourceWidget->saveState(ini);
    _tileGrabberWidget->saveState(ini);
    iniFile.write(ini);
  }
}

void ImOsmDemoApp::beforeLoop() {}

void ImOsmDemoApp::firstPaint() {
  const ImGuiID centralNode{
      ImGui::DockBuilderGetCentralNode(dockSpaceID())->ID};
  ImGui::DockBuilderDockWindow("MapWidget", centralNode);
}


void ImOsmDemoApp::paint() {
  ImGui::Begin("MapWidget");

  ImGui::Text("FPS: %.0f", ImGui::GetIO().Framerate);
  ImGui::Text("MOUSE: lat %.2f, lon %.2f", _mapPlot->mouseLon(),
              _mapPlot->mouseLat());
  ImGui::Text("VIEW: lat %.2f:%.2f, lon %.2f:%.2f ", _mapPlot->minLat(),
              _mapPlot->maxLat(), _mapPlot->minLon(), _mapPlot->maxLon());

  _mapPlot->paint();

  ImGui::End();

  //ShowMarkPanel(_mapPlot);

  // ImGui::Begin("MarkEditor");
  // _distanceCalcWidget->paint();
  // ImGui::Separator();
  // _destinationCalcWidget->paint();
  // ImGui::Separator();
  // _markEditorWidget->paint();

  ImGui::Begin("MarkEditor");
  
  SkanMarks();
  ImGui::End();
  // //_markEditorWidget
  
  // ImGui::End();

  // ImGui::Begin("TileSource");
  // _tileSourceWidget->paint();
  // ImGui::Separator();
  // _tileGrabberWidget->paint();
  // ImGui::End();
}

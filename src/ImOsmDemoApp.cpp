#include "ImOsmDemoApp.h"
#include <imgui_internal.h>
#include "ImOsmMarkRenderer.h"
//#include "ImOsmMarkPanel.h"

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

 ImGuiID leftNode, rightNode;
ImGui::DockBuilderSplitNode(centralNode, ImGuiDir_Left, 0.3f, &leftNode, &rightNode);

ImGuiID topLeftNode, bottomLeftNode;

ImGui::DockBuilderSplitNode(leftNode, ImGuiDir_Up, 0.5f, &topLeftNode, &bottomLeftNode);

ImGui::DockBuilderDockWindow("WayInfo", topLeftNode);
ImGui::DockBuilderDockWindow("Data", bottomLeftNode);
ImGui::DockBuilderDockWindow("MapWidget", rightNode); 

  
}


void ImOsmDemoApp::paint() {

   ImGui::Begin("MapWidget", nullptr, ImGuiWindowFlags_NoResize |
    ImGuiWindowFlags_NoMove |
    ImGuiWindowFlags_NoCollapse |
    ImGuiWindowFlags_NoTitleBar);

   ImGui::Text("FPS: %.0f", ImGui::GetIO().Framerate);
   ImGui::Text("MOUSE: lat %.2f, lon %.2f", _mapPlot->mouseLon(),
               _mapPlot->mouseLat());
   ImGui::Text("VIEW: lat %.2f:%.2f, lon %.2f:%.2f ", _mapPlot->minLat(),
               _mapPlot->maxLat(), _mapPlot->minLon(), _mapPlot->maxLon());

   _mapPlot->paint();

   ImGui::End();

  
   SkanMarks();
  


  
  //ShowMarkPanel(_mapPlot);

  // ImGui::Begin("MarkEditor");
  // _distanceCalcWidget->paint();
  // ImGui::Separator();
  // _destinationCalcWidget->paint();
  // ImGui::Separator();
  // _markEditorWidget->paint();

  
  // //_markEditorWidget
  
  // ImGui::End();

  // ImGui::Begin("TileSource");
  // _tileSourceWidget->paint();
  // ImGui::Separator();
  // _tileGrabberWidget->paint();
  // ImGui::End();
}

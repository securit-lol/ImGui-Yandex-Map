#include "ImOsmMapPlot.h"
#include "ImOsmCoords.h"
#include "ImOsmTileLoaderImpl.h"
#include <algorithm>
#include <implot.h>

namespace ImOsm {
MapPlot::MapPlot() : _loader{std::make_shared<TileLoaderOsmMap>()} {
   addPoint("pidor",0,80);
   getReadyToDraw();
}

MapPlot::MapPlot(std::shared_ptr<ITileLoader> &loader) : _loader{loader} {
  addPoint("pidor",0,80);;
  getReadyToDraw();
  resetBounds();
}

void MapPlot::paint() {
  if (ImPlot::BeginPlot("##ImOsmMapPlot", {-1, -1}, _plotFlags)) {

    ImPlot::SetupAxis(ImAxis_X1, nullptr, _xFlags);
    ImPlot::SetupAxis(ImAxis_Y1, nullptr, _yFlags);
    ImPlot::SetupAxisLimitsConstraints(ImAxis_Y1, 0.0, 1.0);

    if (_setBounds != SetBounds::None) {
      if (_setBounds == SetBounds::Geo) {
        _minX = lon2x(_minLon, 0);
        _maxX = lon2x(_maxLon, 0);
        _minY = lat2y(_minLat, 0);
        _maxY = lat2y(_maxLat, 0);
      } else if (_setBounds == SetBounds::Local) {
        // do nothing
      }
      ImPlot::SetupAxisLimits(ImAxis_X1, _minX, _maxX, ImPlotCond_Always);
      ImPlot::SetupAxisLimits(ImAxis_Y1, _minY, _maxY, ImPlotCond_Always);
      _setBounds = SetBounds::None;
    }

    ImPlot::SetupFinish();

    _mousePos = ImPlot::GetPlotMousePos(ImAxis_X1, ImAxis_Y1);
    _plotLims = ImPlot::GetPlotLimits(ImAxis_X1, ImAxis_Y1);
    _plotSize = ImPlot::GetPlotSize();

    _mouseLon = x2lon(_mousePos.x, 0);
    _mouseLat = y2lat(_mousePos.y, 0);

    _pixelsX = _plotSize.x;
    _pixelsY = _plotSize.y;

    _minX = _plotLims.X.Min;
    _maxX = _plotLims.X.Max;
    _minY = _plotLims.Y.Min;
    _maxY = _plotLims.Y.Max;
    _rangeX = fabs(_maxX - _minX);
    _rangeY = fabs(_maxY - _minY);

    _resX = _pixelsX / _rangeX;
    _resY = _pixelsY / _rangeY;
    _zoom = std::clamp(int(floor(log2(_resX / _tilePixels))), MinZoom, MaxZoom);
    
    float zoomContinuous = log2(_resX / _tilePixels);
    _cur_zoom = std::clamp(zoomContinuous, (float)MinZoom, (float)MaxZoom);

    _tilesNum = POW2[_zoom];
    _tileSize = 1.0 / float(_tilesNum);

    const auto minMaxLat{std::minmax(y2lat(_minY * _tilesNum, _zoom),
                                     y2lat(_maxY * _tilesNum, _zoom))};
    const auto minMaxLon{std::minmax(x2lon(_minX * _tilesNum, _zoom),
                                     x2lon(_maxX * _tilesNum, _zoom))};

    _minLat = minMaxLat.first;
    _maxLat = minMaxLat.second;
    _minLon = minMaxLon.first;
    _maxLon = minMaxLon.second;

    _minTX = std::clamp(int(_minX * _tilesNum), 0, _tilesNum - 1);
    _maxTX = std::clamp(int(_maxX * _tilesNum), 0, _tilesNum - 1);
    _minTY = std::clamp(int(_minY * _tilesNum), 0, _tilesNum - 1);
    _maxTY = std::clamp(int(_maxY * _tilesNum), 0, _tilesNum - 1);

    _loader->beginLoad(_zoom, _minTX, _maxTX, _minTY, _maxTY);

    ImVec2 bmin{float(_minTX), float(_minTY)};
    ImVec2 bmax{float(_maxTX), float(_maxTY)};

    for (auto x{_minTX}; x != _maxTX + 1; ++x) {
      bmin.x = float(x) * _tileSize;
      bmax.x = float(x + 1) * _tileSize;
      for (auto y{_minTY}; y != _maxTY + 1; ++y) {
        bmin.y = float(y) * _tileSize;
        bmax.y = float(y + 1) * _tileSize;
        ImPlot::PlotImage("##", _loader->tileAt(_zoom, x, y), bmin, bmax, _uv0,
                          _uv1, _tint);
      }
    }

    _loader->endLoad();



    float alpha = 0.1f;

    if (_cur_zoom < 4.0f) {
        alpha = 0.1f;
    } 
    else if (_cur_zoom >= 4.0f && _cur_zoom < 8.0f) {
        float t = (_cur_zoom - 4.0f) / (8.0f - 4.0f);
         alpha = 0.1f + t * 0.9f;
    } 
    else {
        alpha = 1.0f;
    }

    if (alpha > 0.0f) {
    for (auto _obj :  *getPoints()) {
        double x = _obj.second.x;
        double y = _obj.second.y;
    
    ImPlot::SetNextMarkerStyle(ImPlotMarker_Circle, 8.0f, ImVec4(1, 0, 0, alpha), 1.5f, ImVec4(1, 1, 1, 1));
    
    ImPlot::PlotScatter("Point", &x, &y, 1);
    }
  }

    ImPlot::EndPlot();
  }
}
} // namespace ImOsm

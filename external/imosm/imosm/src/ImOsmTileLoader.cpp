#include "ImOsmTileLoader.h"
#include "ImOsmITile.h"
#include "ImOsmITileSource.h"
#include <algorithm>

namespace ImOsm {
TileLoader::TileLoader(std::shared_ptr<ITileSource> source) : _source{source} {}

void ImOsm::TileLoader::beginLoad(int z, int xmin, int xmax, int ymin,
                                  int ymax) {
  _currentZoom = z;
  
  const auto cond{[](const std::shared_ptr<ITile> &tile) {
    return tile->isDummy();
  }};
  _tiles.erase(std::remove_if(_tiles.begin(), _tiles.end(), cond),
               _tiles.end());
  _source->takeReady(_tiles);

  enforceCacheLimit();
}

void ImOsm::TileLoader::enforceCacheLimit() {
  if (static_cast<int>(_tiles.size()) > MAX_TILES_IN_CACHE) {
    std::sort(_tiles.begin(), _tiles.end(),
              [this](const std::shared_ptr<ITile> &a,
                     const std::shared_ptr<ITile> &b) {
                int zoomA = a->z();
                int zoomB = b->z();
                int distA = std::abs(zoomA - _currentZoom);
                int distB = std::abs(zoomB - _currentZoom);
                if (distA != distB) return distA < distB;
                return false;
              });
    
    int toRemove = static_cast<int>(_tiles.size()) - MAX_TILES_IN_CACHE * 90 / 100;
    if (toRemove > 0) {
      _tiles.erase(_tiles.begin() + _tiles.size() - toRemove, _tiles.end());
    }
  }
}

ImTextureID TileLoader::tileAt(int z, int x, int y) {
  const auto cond{[z, x, y](const std::shared_ptr<ITile> &tile) {
    return tile->isTileZXY(z, x, y);
  }};
  const auto it{std::find_if(_tiles.begin(), _tiles.end(), cond)};

  if (it != _tiles.end()) {
    if (!(*it)->isDummy()) {
      return (*it)->texture();
    }
  }

  if (!_source->hasRequest(z, x, y)) {
    _source->request(z, x, y);
  }

  for (int fallbackZ = z - 1; fallbackZ >= 0; --fallbackZ) {
    int fallbackX = x >> (z - fallbackZ);
    int fallbackY = y >> (z - fallbackZ);
    
    const auto fallbackCond{[fallbackZ, fallbackX, fallbackY](
                                const std::shared_ptr<ITile> &tile) {
      return tile->isTileZXY(fallbackZ, fallbackX, fallbackY);
    }};
    const auto fallbackIt{
        std::find_if(_tiles.begin(), _tiles.end(), fallbackCond)};
    
    if (fallbackIt != _tiles.end() && !(*fallbackIt)->isDummy()) {
      return (*fallbackIt)->texture();
    }
  }
  return 0;
}
} // namespace ImOsm

#pragma once
#include "ImOsmITileLoader.h"
#include <memory>
#include <vector>
#include <deque>

namespace ImOsm {
class ITile;
class ITileSource;

class TileLoader : public ITileLoader {
public:
  TileLoader(std::shared_ptr<ITileSource> source);
  virtual ~TileLoader() = default;

  virtual void beginLoad(int z, int xmin, int xmax, int ymin,
                         int ymax) override;
  virtual ImTextureID tileAt(int z, int x, int y) override;
  virtual int tileCount() const override { return _tiles.size(); }
  virtual void endLoad() override {}

  const std::vector<std::shared_ptr<ITile>>& getTiles() const {
    return _tiles;
  }

private:
  void enforceCacheLimit();

  std::shared_ptr<ITileSource> _source;
  std::vector<std::shared_ptr<ITile>> _tiles;
  std::deque<std::pair<int, std::shared_ptr<ITile>>> _tileHistory;  // Track tile usage order
  int _currentZoom{0};
  static constexpr int MAX_TILES_IN_CACHE{512};  // Limit cache to ~256MB (512 tiles * ~500KB each)
};
}; // namespace ImOsm

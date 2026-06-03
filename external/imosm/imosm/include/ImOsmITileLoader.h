#pragma once

#include <vector>
#include <memory>

namespace ImOsm {
using ImTextureID = void *;
class ITile;

class ITileLoader {
public:
  virtual ~ITileLoader() = default;

  virtual void beginLoad(int z, int xmin, int xmax, int ymin, int ymax) {}
  virtual ImTextureID tileAt(int z, int x, int y) = 0;
  virtual int tileCount() const { return 0; };
  // Return reference to currently loaded tiles (may be empty)
  virtual const std::vector<std::shared_ptr<ITile>>& getTiles() const = 0;
  virtual void endLoad() {}
};
} // namespace ImOsm

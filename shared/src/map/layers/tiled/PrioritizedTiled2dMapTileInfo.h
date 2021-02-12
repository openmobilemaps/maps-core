#pragma once

#include "Tiled2dMapTileInfo.h"
#include <functional>

struct PrioritizedTiled2dMapTileInfo {
  Tiled2dMapTileInfo tileInfo;
  int priority;

  PrioritizedTiled2dMapTileInfo(Tiled2dMapTileInfo tileInfo, int priority)
          : tileInfo(tileInfo), priority(priority) {}

  bool operator==(const PrioritizedTiled2dMapTileInfo &o) const {
    return tileInfo == o.tileInfo;
  }

  bool operator<(const PrioritizedTiled2dMapTileInfo &o) const {
      return (priority < o.priority) || (priority == o.priority && tileInfo < o.tileInfo);
  }
};

namespace std {
    template<>
    struct hash<PrioritizedTiled2dMapTileInfo> {
        inline size_t operator()(const PrioritizedTiled2dMapTileInfo& tileInfo) const {
          return std::hash<Tiled2dMapTileInfo>()(tileInfo.tileInfo);
        }
    };
}

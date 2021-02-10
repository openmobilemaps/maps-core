//
// Created by Christoph Maurhofer on 09.02.2021.
//

#pragma once

#include "RectCoord.h"
#include <stdint.h>

struct Tiled2dMapTileInfo {
    RectCoord bounds;
    int x;
    int y;
    int zoom;
    int loadingPriority; // priorities > 0, if must be loaded

    Tiled2dMapTileInfo(RectCoord bounds, int x, int y, int zoom, int loadingPriority)
            : bounds(bounds), x(x), y(y), zoom(zoom), loadingPriority(loadingPriority) {}

    bool operator==(const Tiled2dMapTileInfo &o) const {
        return x == o.x && y == o.y && zoom == o.zoom && loadingPriority == o.loadingPriority;
    }

    bool operator<(const Tiled2dMapTileInfo &o) const {
        return x < o.x || (x == o.x && y < o.y) || (x == o.x && y == o.y && zoom < o.zoom) ||
               (x == o.x && y == o.y && zoom == o.zoom && loadingPriority < o.loadingPriority);
    }
};

namespace std {
    template<>
    struct hash<Tiled2dMapTileInfo> {
        inline size_t operator()(const Tiled2dMapTileInfo& tileInfo) const {
            int sizeBits = (SIZE_MAX == 0xFFFFFFFF) ? 32 : 64;
            size_t hash = ((size_t) tileInfo.x << (2 * sizeBits / 3)) | ((size_t) tileInfo.y << (sizeBits / 3)) | ((size_t) tileInfo.zoom) ;
            return hash;
        }
    };
}
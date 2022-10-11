/*
 * Copyright (c) 2021 Ubique Innovation AG <https://www.ubique.ch>
 *
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this
 *  file, You can obtain one at https://mozilla.org/MPL/2.0/.
 *
 *  SPDX-License-Identifier: MPL-2.0
 */

#pragma once

#include "RectCoord.h"
#include <stdint.h>
#include "HashedTuple.h"
#include "Tiled2dMapTileInfo.h"

struct Tiled2dMapTileInfo {
    RectCoord bounds;
    int x;
    int y;
    int t;
    int zoomIdentifier;
    int zoomLevel;

    Tiled2dMapTileInfo(RectCoord bounds, int x, int y, int z, int zoomIdentifier, int zoomLevel)
        : bounds(bounds)
        , x(x)
        , y(y)
        , t(z)
        , zoomIdentifier(zoomIdentifier)
        , zoomLevel(zoomLevel) {}

    bool operator==(const Tiled2dMapTileInfo &o) const { return x == o.x && y == o.y  && t == o.t && zoomIdentifier == o.zoomIdentifier; }

    bool operator!=(const Tiled2dMapTileInfo &o) const { return !(x == o.x || y == o.y || t == o.t || zoomIdentifier == o.zoomIdentifier); }

    bool operator<(const Tiled2dMapTileInfo &o) const {
        return zoomIdentifier < o.zoomIdentifier || (zoomIdentifier == o.zoomIdentifier && x < o.x) ||
               (zoomIdentifier == o.zoomIdentifier && x == o.x && y < o.y) ||
                (zoomIdentifier == o.zoomIdentifier && x == o.x && y == o.y && t < o.t);
    }
};

namespace std {
template <> struct hash<Tiled2dMapTileInfo> {
    inline size_t operator()(const Tiled2dMapTileInfo &k) const {
        size_t res = 17;
        res = res * 31 + std::hash<int>()( k.x );
        res = res * 31 + std::hash<int>()( k.y );
        res = res * 31 + std::hash<int>()( k.t );
        res = res * 31 + std::hash<int>()( k.zoomIdentifier );
        return res;
    }
};
} // namespace std

struct TileLoadTask {
    Tiled2dMapTileInfo tileInfo;
    int subtaskIndex;

    TileLoadTask(Tiled2dMapTileInfo tileInfo, int subtaskIndex) :
    tileInfo(tileInfo), subtaskIndex(subtaskIndex) {}

    bool operator==(const TileLoadTask &o) const { return tileInfo == o.tileInfo && subtaskIndex == o.subtaskIndex; }

    bool operator!=(const TileLoadTask &o) const { return tileInfo != o.tileInfo || subtaskIndex != o.subtaskIndex;  }

    bool operator<(const TileLoadTask &o) const {
        return tileInfo < o.tileInfo || subtaskIndex < o.subtaskIndex;
    }
};

namespace std {
    template <> struct hash<TileLoadTask> {
        inline size_t operator()(const TileLoadTask &k) const {
            auto h = std::hash<Tiled2dMapTileInfo>()(k.tileInfo);
            std::hash_combine(h, std::hash<int>{}(k.subtaskIndex));
            return h;
        }
    };
} // namespace std

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
#include <string>

struct Tiled2dMapTileInfo {
    RectCoord bounds;
    int x;
    int y;
    int t;
    int zoomIdentifier;
    int zoomLevel;
    int tessellationFactor = 0;

    Tiled2dMapTileInfo(RectCoord bounds, int x, int y, int z, int zoomIdentifier, int zoomLevel)
        : bounds(bounds)
        , x(x)
        , y(y)
        , t(z)
        , zoomIdentifier(zoomIdentifier)
        , zoomLevel(zoomLevel) {}

    bool operator==(const Tiled2dMapTileInfo &o) const {
        return x == o.x && y == o.y  && t == o.t && zoomIdentifier == o.zoomIdentifier;
    }

    auto keyTupleWithoutT() const {
        return std::tuple { x, y, zoomIdentifier };
    }

    bool operator!=(const Tiled2dMapTileInfo &o) const {
        return !(x == o.x && y == o.y  && t == o.t && zoomIdentifier == o.zoomIdentifier);
    }

    bool operator<(const Tiled2dMapTileInfo &o) const {
        return zoomIdentifier < o.zoomIdentifier || (zoomIdentifier == o.zoomIdentifier && x < o.x) ||
               (zoomIdentifier == o.zoomIdentifier && x == o.x && y < o.y) ||
                (zoomIdentifier == o.zoomIdentifier && x == o.x && y == o.y && t < o.t);
    }

    std::string to_string() const {
        return "Tiled2dMapTileInfo(" + std::to_string(zoomIdentifier) + "/" + std::to_string(x)  + "/" + std::to_string(y) + "/" + std::to_string(t) + ")";
    }

    std::string to_string_short() const {
        return std::to_string(zoomIdentifier) + "/" + std::to_string(x)  + "/" + std::to_string(y) + "/" + std::to_string(t);
    }
};

inline std::ostream& operator<<(std::ostream& os, const Tiled2dMapTileInfo& tile)
{
    return os << tile.to_string();
}

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

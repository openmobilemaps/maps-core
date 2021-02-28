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

struct Tiled2dMapTileInfo {
    RectCoord bounds;
    int x;
    int y;
    int zoomIdentifier;
    int zoomLevel;

    Tiled2dMapTileInfo(RectCoord bounds, int x, int y, int zoomIdentifier, int zoomLevel)
        : bounds(bounds)
        , x(x)
        , y(y)
        , zoomIdentifier(zoomIdentifier)
        , zoomLevel(zoomLevel) {}

    bool operator==(const Tiled2dMapTileInfo &o) const { return x == o.x && y == o.y && zoomIdentifier == o.zoomIdentifier; }

    bool operator<(const Tiled2dMapTileInfo &o) const {
        return x < o.x || (x == o.x && y < o.y) || (x == o.x && y == o.y && zoomIdentifier < o.zoomIdentifier) ||
               (x == o.x && y == o.y && zoomIdentifier == o.zoomIdentifier);
    }
};

namespace std {
template <> struct hash<Tiled2dMapTileInfo> {
    inline size_t operator()(const Tiled2dMapTileInfo &tileInfo) const {
        int sizeBits = (SIZE_MAX == 0xFFFFFFFF) ? 32 : 64;
        size_t hash =
            ((size_t)tileInfo.x << (2 * sizeBits / 3)) | ((size_t)tileInfo.y << (sizeBits / 3)) | ((size_t)tileInfo.zoomIdentifier);
        return hash;
    }
};
} // namespace std

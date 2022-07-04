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

#include "Tiled2dMapTileInfo.h"
#include "Value.h"
#include "PolygonCoord.h"
#include "VectorTileGeometryHandler.h"
#include <functional>

struct Tiled2dMapVectorTileInfo {
    const Tiled2dMapTileInfo tileInfo;
    const std::shared_ptr<std::unordered_map<std::string, std::vector<std::tuple<const FeatureContext, const VectorTileGeometryHandler>>>> layerFeatureMap;
    PolygonCoord mask;

    Tiled2dMapVectorTileInfo(Tiled2dMapTileInfo tileInfo,
                             const std::shared_ptr<std::unordered_map<std::string, std::vector<std::tuple<const FeatureContext, const VectorTileGeometryHandler>>>> &layerFeatureMap,
                             const PolygonCoord mask)
        : tileInfo(tileInfo)
        , layerFeatureMap(layerFeatureMap)
        , mask(mask) {}

    bool operator==(const Tiled2dMapVectorTileInfo &o) const { return tileInfo == o.tileInfo; }

    bool operator<(const Tiled2dMapVectorTileInfo &o) const { return tileInfo < o.tileInfo; }
};

namespace std {
template <> struct hash<Tiled2dMapVectorTileInfo> {
    inline size_t operator()(const Tiled2dMapVectorTileInfo &tileInfo) const {
        return std::hash<Tiled2dMapTileInfo>()(tileInfo.tileInfo);
    }
};
} // namespace std

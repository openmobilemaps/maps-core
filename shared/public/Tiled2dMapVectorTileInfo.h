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
#include "Tiled2dMapTileInfo.h"

struct Tiled2dMapVectorTileInfo {
    const TileLoadTask tileInfo;
    const std::unordered_map<std::string, std::shared_ptr<std::unordered_map<std::string, std::vector<std::tuple<const FeatureContext, const VectorTileGeometryHandler>>>>> layerFeatureMaps;
    std::vector<::PolygonCoord> masks;

    Tiled2dMapVectorTileInfo(TileLoadTask tileInfo,
                             const std::unordered_map<std::string, std::shared_ptr<std::unordered_map<std::string, std::vector<std::tuple<const FeatureContext, const VectorTileGeometryHandler>>>>> &layerFeatureMaps,
                             const std::vector<::PolygonCoord> &masks)
        : tileInfo(tileInfo)
        , layerFeatureMaps(layerFeatureMaps)
        , masks(masks) {}

    bool operator==(const Tiled2dMapVectorTileInfo &o) const { return tileInfo == o.tileInfo; }

    bool operator<(const Tiled2dMapVectorTileInfo &o) const { return tileInfo < o.tileInfo; }
};

namespace std {
template <> struct hash<Tiled2dMapVectorTileInfo> {
    inline size_t operator()(const Tiled2dMapVectorTileInfo &tileInfo) const {
        return std::hash<Tiled2dMapTileInfo>()(tileInfo.tileInfo.tileInfo);
    }
};
} // namespace std

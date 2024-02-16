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

#include "Tiled2dMapVersionedTileInfo.h"
#include "Value.h"
#include "PolygonCoord.h"
#include "VectorTileGeometryHandler.h"
#include <functional>
#include "TileState.h"

struct Tiled2dMapVectorTileInfo {
    using FeatureTuple = std::tuple<const std::shared_ptr<FeatureContext>, const std::shared_ptr<VectorTileGeometryHandler>>;
    using FeatureMap = std::shared_ptr<std::unordered_map<std::string, std::shared_ptr<std::vector<FeatureTuple>>>>;

    const Tiled2dMapVersionedTileInfo tileInfo;
    const FeatureMap layerFeatureMaps;
    const std::vector<::PolygonCoord> masks;
    const TileState state;

    Tiled2dMapVectorTileInfo(Tiled2dMapVersionedTileInfo tileInfo,
                             const FeatureMap &layerFeatureMaps,
                             const std::vector<::PolygonCoord> &masks,
                             const TileState state)
        : tileInfo(tileInfo)
        , layerFeatureMaps(layerFeatureMaps)
        , masks(masks)
        , state(state) {}

    bool operator==(const Tiled2dMapVectorTileInfo &o) const { return tileInfo == o.tileInfo && layerFeatureMaps.get() == o.layerFeatureMaps.get(); }

    bool operator<(const Tiled2dMapVectorTileInfo &o) const { return tileInfo < o.tileInfo; }
};

namespace std {
template <> struct hash<Tiled2dMapVectorTileInfo> {
    inline size_t operator()(const Tiled2dMapVectorTileInfo &tileInfo) const {
        return std::hash<Tiled2dMapVersionedTileInfo>()(tileInfo.tileInfo);
    }
};
} // namespace std

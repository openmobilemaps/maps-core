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

#include "TextureHolderInterface.h"
#include "PolygonCoord.h"
#include "Tiled2dMapVersionedTileInfo.h"
#include "Tiled2dMapTileInfo.h"
#include <functional>
#include "TileState.h"

struct Tiled2dMapRasterTileInfo {
    Tiled2dMapVersionedTileInfo tileInfo;
    std::shared_ptr<TextureHolderInterface> textureHolder;
    std::vector<::PolygonCoord> masks;
    TileState state;
    int tessellationFactor;

    Tiled2dMapRasterTileInfo(Tiled2dMapVersionedTileInfo tileInfo, const std::shared_ptr<TextureHolderInterface> textureHolder, const std::vector<::PolygonCoord> masks, const TileState state, int tessellationFactor)
        : tileInfo(tileInfo)
        , textureHolder(textureHolder)
        , masks(masks)
        , state(state)
        , tessellationFactor(tessellationFactor) {}

    void updateMasks(const std::vector<::PolygonCoord> & masks){
        this->masks = masks;
    }

    bool operator==(const Tiled2dMapRasterTileInfo &o) const { return tileInfo == o.tileInfo; }

    bool operator<(const Tiled2dMapRasterTileInfo &o) const { return tileInfo < o.tileInfo; }
};

namespace std {
template <> struct hash<Tiled2dMapRasterTileInfo> {
    inline size_t operator()(const Tiled2dMapRasterTileInfo &tileInfo) const {
        return std::hash<Tiled2dMapVersionedTileInfo>()(tileInfo.tileInfo);
    }
};
} // namespace std

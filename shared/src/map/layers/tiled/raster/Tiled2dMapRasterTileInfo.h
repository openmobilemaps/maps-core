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
#include "Tiled2dMapTileInfo.h"
#include <functional>

struct Tiled2dMapRasterTileInfo {
    Tiled2dMapTileInfo tileInfo;
    std::shared_ptr<TextureHolderInterface> textureHolder;

    Tiled2dMapRasterTileInfo(Tiled2dMapTileInfo tileInfo, const std::shared_ptr<TextureHolderInterface> textureHolder)
        : tileInfo(tileInfo)
        , textureHolder(textureHolder) {}

    bool operator==(const Tiled2dMapRasterTileInfo &o) const { return tileInfo == o.tileInfo; }

    bool operator<(const Tiled2dMapRasterTileInfo &o) const { return tileInfo < o.tileInfo; }
};

namespace std {
template <> struct hash<Tiled2dMapRasterTileInfo> {
    inline size_t operator()(const Tiled2dMapRasterTileInfo &tileInfo) const {
        return std::hash<Tiled2dMapTileInfo>()(tileInfo.tileInfo);
    }
};
} // namespace std
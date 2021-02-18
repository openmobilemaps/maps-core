//
// Created by Christoph Maurhofer on 10.02.2021.
//

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
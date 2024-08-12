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
#include <stdint.h>
#include <string>

struct Tiled2dMapVersionedTileInfo {
    Tiled2dMapTileInfo tileInfo;
    size_t tileVersion;

    Tiled2dMapVersionedTileInfo(Tiled2dMapTileInfo tileInfo, size_t tileVersion)
    : tileInfo(tileInfo)
    , tileVersion(tileVersion)
     {}

    bool operator==(const Tiled2dMapVersionedTileInfo &o) const {
        return tileInfo == o.tileInfo && tileVersion == o.tileVersion;
    }

    bool operator!=(const Tiled2dMapVersionedTileInfo &o) const {
        return !(tileInfo == o.tileInfo && tileVersion == o.tileVersion);
    }

    bool operator<(const Tiled2dMapVersionedTileInfo &o) const {
        return tileInfo < o.tileInfo;
    }

    std::string to_string() const {
        return "Tiled2dMapVersionedTileInfo(" + tileInfo.to_string() + "@" + std::to_string(tileVersion) + ")";
    }
};

namespace std {
    template <> struct hash<Tiled2dMapVersionedTileInfo> {
        inline size_t operator()(const Tiled2dMapVersionedTileInfo &k) const {
            size_t res = std::hash<Tiled2dMapTileInfo>()( k.tileInfo );
            res = res * 31 + std::hash<size_t>()( k.tileVersion );
            return res;
        }
    };
} // namespace std

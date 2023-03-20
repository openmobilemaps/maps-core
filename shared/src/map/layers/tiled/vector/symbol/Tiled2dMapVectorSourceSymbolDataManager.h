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

#include "Tiled2dMapVectorSourceTileDataManager.h"
#include "Tiled2dMapVectorSource.h"
#include "Tiled2dMapVectorSymbolFeatureWrapper.h"
#include "Tiled2dMapVectorSourceDataManager.h"

class Tiled2dMapVectorSourceSymbolDataManager: public Tiled2dMapVectorSourceDataManager {
public:
    using Tiled2dMapVectorSourceDataManager::Tiled2dMapVectorSourceDataManager;

    void onVectorTilesUpdated(const std::string &sourceName, std::unordered_set<Tiled2dMapVectorTileInfo> currentTileInfos) override;

    //cached locked unsafe renderpasses

//    void collisionDetection(LayerIdentifier, &std::vector<Placements>)
//
//    std::map<TileInfo, std::map<LayerIdentifier, std::vector<Symbols>>>
};

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

class Tiled2dMapVectorSourceVectorTileDataManager : public Tiled2dMapVectorSourceTileDataManager {
public:
    Tiled2dMapVectorSourceVectorTileDataManager(const WeakActor<Tiled2dMapVectorLayer> &vectorLayer,
                                                const std::shared_ptr<VectorMapDescription> &mapDescription,
                                                const std::string &source,
                                                const WeakActor<Tiled2dMapVectorSource> &vectorSource);

    void onVectorTilesUpdated(const std::string &sourceName, std::unordered_set<Tiled2dMapVectorTileInfo> currentTileInfos) override;

    virtual void updateLayerDescription(std::shared_ptr<VectorLayerDescription> layerDescription,
                                        int32_t legacyIndex,
                                        bool needsTileReplace) override;


protected:
    void onTileCompletelyReady(const Tiled2dMapTileInfo tileInfo) override;

private:
    void clearTiles(const std::vector<Actor<Tiled2dMapVectorTile>> &tilesToClear);

    const WeakActor<Tiled2dMapVectorSource> vectorSource;
};

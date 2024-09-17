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
#include "Tiled2dMapRasterSource.h"

class Tiled2dMapVectorSourceRasterTileDataManager : public Tiled2dMapVectorSourceTileDataManager {
public:
    Tiled2dMapVectorSourceRasterTileDataManager(const WeakActor<Tiled2dMapVectorLayer> &vectorLayer,
                                                const std::shared_ptr<VectorMapDescription> &mapDescription,
                                                const std::shared_ptr<Tiled2dMapVectorLayerConfig> &layerConfig,
                                                const std::shared_ptr<VectorLayerDescription> &layerDescription,
                                                const WeakActor<Tiled2dMapRasterSource> &rasterSource,
                                                const Actor<Tiled2dMapVectorReadyManager> &readyManager,
                                                const std::shared_ptr<Tiled2dMapVectorStateManager> &featureStateManager,
                                                double dpFactor);

    void onRasterTilesUpdated(const std::string &layerName, std::unordered_set<Tiled2dMapRasterTileInfo> currentTileInfos) override;

    virtual void updateLayerDescription(std::shared_ptr<VectorLayerDescription> layerDescription,
                                        int32_t legacyIndex,
                                        bool needsTileReplace) override;

    virtual void reloadLayerContent(const std::vector<std::tuple<std::shared_ptr<VectorLayerDescription>, int32_t>> &descriptionLayerIndexPairs) override;

protected:
    void onTileCompletelyReady(const Tiled2dMapVersionedTileInfo tileInfo) override;

private:
    void clearTiles(const std::vector<Actor<Tiled2dMapVectorTile>> &tilesToClear);

    const std::shared_ptr<VectorLayerDescription> layerDescription;
    const WeakActor<Tiled2dMapRasterSource> rasterSource;
    const double dpFactor = 1.0;
};

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

#include "Tiled2dMapVectorSourceDataManager.h"
#include "Tiled2dMapLayerMaskWrapper.h"
#include "Tiled2dMapVectorLayerReadyInterface.h"

class Tiled2dMapVectorTile;

class Tiled2dMapVectorSourceTileDataManager : public Tiled2dMapVectorSourceDataManager,
                                              public Tiled2dMapVectorLayerReadyInterface,
                                              public std::enable_shared_from_this<Tiled2dMapVectorSourceTileDataManager> {
public:
    Tiled2dMapVectorSourceTileDataManager(const WeakActor<Tiled2dMapVectorLayer> &vectorLayer,
                                          const std::shared_ptr<VectorMapDescription> &mapDescription,
                                          const std::string &source);

    virtual void update() override;

    virtual std::vector<std::tuple<int32_t, std::vector<std::shared_ptr<RenderPassInterface>>>> buildRenderPasses() override;

    virtual void pause() override;

    virtual void resume() override;

    virtual void setAlpha(float alpha) override;

    virtual void tileIsReady(const Tiled2dMapTileInfo &tile) override;

    virtual void updateLayerDescription(std::shared_ptr<VectorLayerDescription> layerDescription) override;

    void setSelectionDelegate(const WeakActor<Tiled2dMapVectorLayerSelectionInterface> &selectionDelegate) override;

    void setSelectedFeatureIdentifier(std::optional<int64_t> identifier) override;

    virtual void onRasterTilesUpdated(const std::string &layerName, std::unordered_set<Tiled2dMapRasterTileInfo> currentTileInfos) override {};

    virtual void onVectorTilesUpdated(const std::string &sourceName, std::unordered_set<Tiled2dMapVectorTileInfo> currentTileInfos) override {};

    void updateMaskObjects(const std::unordered_map<Tiled2dMapTileInfo, Tiled2dMapLayerMaskWrapper> toSetupMaskObject,
                           const std::unordered_set<Tiled2dMapTileInfo> tilesToRemove);

protected:
    Actor<Tiled2dMapVectorTile> createTileActor(const Tiled2dMapTileInfo &tileInfo,
                                                const std::shared_ptr<VectorLayerDescription> &layerDescription);

    virtual void onTileCompletelyReady(const Tiled2dMapTileInfo tileInfo) = 0;

    std::unordered_map<Tiled2dMapTileInfo, std::vector<std::tuple<int32_t, std::string, Actor<Tiled2dMapVectorTile>>>> tiles;
    std::unordered_map<Tiled2dMapTileInfo, Tiled2dMapLayerMaskWrapper> tileMaskMap;
    std::unordered_set<Tiled2dMapTileInfo> tilesReady;
    std::unordered_map<Tiled2dMapTileInfo, int> tilesReadyCount;
};
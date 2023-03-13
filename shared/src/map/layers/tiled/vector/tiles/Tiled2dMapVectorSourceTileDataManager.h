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
#include "Tiled2dMapVectorLayer.h"
#include "Tiled2dMapVectorSourceDataManager.h"
#include "Actor.h"
#include <unordered_set>

class Tiled2dMapVectorSourceTileDataManager : Tiled2dMapVectorSourceDataManager {
public:
    Tiled2dMapVectorSourceTileDataManager(const WeakActor<Tiled2dMapVectorLayer> &vectorLayer);

    virtual void update() override;

    virtual std::vector<std::tuple<int32_t, std::vector<std::shared_ptr<RenderObjectInterface>>>> getRenderObjects() override;

    virtual void onTilesUpdated(const std::string &layerName, std::unordered_set<Tiled2dMapRasterTileInfo> currentTileInfos) override;

    virtual void onTilesUpdated(const std::string &sourceName, std::unordered_set<Tiled2dMapVectorTileInfo> currentTileInfos) override;

    virtual void onAdded(const std::weak_ptr<::MapInterface> &mapInterface) override;

    virtual void onRemoved() override;

    virtual void pause() override;

    virtual void resume() override;

    virtual void setAlpha(float alpha) override;

    virtual void tileIsReady(const Tiled2dMapTileInfo &tile) override;

    virtual void updateLayerDescription(std::shared_ptr<VectorLayerDescription> layerDescription) override;

private:
    void updateMaskObjects(const std::unordered_map<Tiled2dMapTileInfo, Tiled2dMapLayerMaskWrapper> toSetupMaskObject, const std::unordered_set<Tiled2dMapTileInfo> tilesToRemove);

    const WeakActor<Tiled2dMapVectorLayer> vectorLayer;

    std::unordered_map<Tiled2dMapTileInfo, std::vector<std::tuple<int32_t, std::string, Actor<Tiled2dMapVectorTile>>>> tiles;
};
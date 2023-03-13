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
#include "Actor.h"
#include <unordered_set>

class Tiled2dMapVectorSourceDataManager {
public:
    Tiled2dMapVectorSourceDataManager(const WeakActor<Tiled2dMapVectorLayer> &vectorLayer);

    virtual void update() = 0;

    virtual std::vector<std::tuple<int32_t, std::vector<std::shared_ptr<RenderObjectInterface>>>> getRenderObjects() = 0;

    virtual void onTilesUpdated(const std::string &layerName, std::unordered_set<Tiled2dMapRasterTileInfo> currentTileInfos) = 0;

    virtual void onTilesUpdated(const std::string &sourceName, std::unordered_set<Tiled2dMapVectorTileInfo> currentTileInfos) = 0;

    virtual void onAdded(const std::weak_ptr<::MapInterface> &mapInterface);

    virtual void onRemoved();

    virtual void pause() = 0;

    virtual void resume() = 0;

    virtual void setAlpha(float alpha) = 0;

    virtual void tileIsReady(const Tiled2dMapTileInfo &tile) = 0;

    virtual void updateLayerDescription(std::shared_ptr<VectorLayerDescription> layerDescription) = 0;

private:
    const WeakActor<Tiled2dMapVectorLayer> vectorLayer;
    std::weak_ptr<MapInterface> mapInterface;
};
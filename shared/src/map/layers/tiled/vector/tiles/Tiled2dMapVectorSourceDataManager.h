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
#include "VectorMapDescription.h"
#include "RenderPassInterface.h"
#include "Tiled2dMapRasterTileInfo.h"
#include "Tiled2dMapVectorTileInfo.h"
#include "MapInterface.h"
#include "Tiled2dMapVectorLayerSelectionInterface.h"
#include "Actor.h"
#include <unordered_set>

class Tiled2dMapVectorLayer;

class Tiled2dMapVectorSourceDataManager : public ActorObject {
public:
    Tiled2dMapVectorSourceDataManager(const WeakActor<Tiled2dMapVectorLayer> &vectorLayer,
                                      const std::shared_ptr<VectorMapDescription> &mapDescription,
                                      const std::string &source);

    virtual void update() = 0;

    virtual std::vector<std::tuple<int32_t, std::vector<std::shared_ptr<RenderPassInterface>>>> buildRenderPasses() = 0;

    virtual void onRasterTilesUpdated(const std::string &layerName, std::unordered_set<Tiled2dMapRasterTileInfo> currentTileInfos) = 0;

    virtual void onVectorTilesUpdated(const std::string &sourceName, std::unordered_set<Tiled2dMapVectorTileInfo> currentTileInfos) = 0;

    virtual void onAdded(const std::weak_ptr<::MapInterface> &mapInterface);

    virtual void onRemoved();

    virtual void pause() = 0;

    virtual void resume() = 0;

    virtual void setSelectionDelegate(const WeakActor<Tiled2dMapVectorLayerSelectionInterface> &selectionDelegate);

    virtual void setSelectedFeatureIdentifier(std::optional<int64_t> identifier) = 0;

    virtual void setAlpha(float alpha) = 0;

    virtual void updateLayerDescription(std::shared_ptr<VectorLayerDescription> layerDescription) = 0;

protected:
    std::weak_ptr<MapInterface> mapInterface;
    const WeakActor<Tiled2dMapVectorLayer> vectorLayer;
    const std::shared_ptr<VectorMapDescription> mapDescription;
    const std::string source;
    WeakActor<Tiled2dMapVectorLayerSelectionInterface> selectionDelegate;
};
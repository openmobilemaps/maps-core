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

#include "MapInterface.h"
#include "Tiled2dMapTileInfo.h"
#include "VectorMapDescription.h"
#include "RenderPassInterface.h"
#include "Tiled2dMapRasterTileInfo.h"
#include "Tiled2dMapVectorTileInfo.h"
#include "Tiled2dMapVectorLayerSelectionInterface.h"
#include "Tiled2dMapVectorLayerReadyInterface.h"
#include "Tiled2dMapLayerMaskWrapper.h"
#include "Actor.h"
#include <unordered_set>

class Tiled2dMapVectorLayer;
class Tiled2dMapVectorTile;

class Tiled2dMapVectorSourceTileDataManager : public Tiled2dMapVectorLayerReadyInterface,
                                              public ActorObject,
                                              public std::enable_shared_from_this<Tiled2dMapVectorSourceTileDataManager> {
public:
    Tiled2dMapVectorSourceTileDataManager(const WeakActor<Tiled2dMapVectorLayer> &vectorLayer,
                                          const std::shared_ptr<VectorMapDescription> &mapDescription,
                                          const std::string &source);

    virtual void onAdded(const std::weak_ptr<::MapInterface> &mapInterface);

    virtual void onRemoved();

    virtual void update();

    virtual void pause();

    virtual void resume();

    virtual void setAlpha(float alpha);

    virtual void setScissorRect(const std::optional<RectI> &scissorRect);

    virtual void tileIsReady(const Tiled2dMapTileInfo &tile, const std::string &layerIdentifier,
                             const std::vector<std::shared_ptr<RenderObjectInterface>> renderObjects) override;

    virtual void updateLayerDescription(std::shared_ptr<VectorLayerDescription> layerDescription);

    void setSelectionDelegate(const WeakActor<Tiled2dMapVectorLayerSelectionInterface> &selectionDelegate);

    void setSelectedFeatureIdentifier(std::optional<int64_t> identifier);

    virtual void onRasterTilesUpdated(const std::string &layerName, std::unordered_set<Tiled2dMapRasterTileInfo> currentTileInfos) {};

    virtual void onVectorTilesUpdated(const std::string &sourceName, std::unordered_set<Tiled2dMapVectorTileInfo> currentTileInfos) {};

    void updateMaskObjects(const std::unordered_map<Tiled2dMapTileInfo, Tiled2dMapLayerMaskWrapper> toSetupMaskObject,
                           const std::unordered_set<Tiled2dMapTileInfo> tilesToRemove);

protected:
    Actor<Tiled2dMapVectorTile> createTileActor(const Tiled2dMapTileInfo &tileInfo,
                                                const std::shared_ptr<VectorLayerDescription> &layerDescription);

    virtual void pregenerateRenderPasses();

    virtual void onTileCompletelyReady(const Tiled2dMapTileInfo tileInfo) = 0;

    std::weak_ptr<MapInterface> mapInterface;
    const WeakActor<Tiled2dMapVectorLayer> vectorLayer;
    const std::shared_ptr<VectorMapDescription> mapDescription;
    const std::string source;
    WeakActor<Tiled2dMapVectorLayerSelectionInterface> selectionDelegate;

    std::optional<::RectI> scissorRect = std::nullopt;

    std::unordered_map<Tiled2dMapTileInfo, std::vector<std::tuple<int32_t, std::vector<std::shared_ptr<RenderObjectInterface>>>>> tileRenderObjectsMap;
    std::unordered_map<std::string, int32_t> layerNameIndexMap;
    std::unordered_map<Tiled2dMapTileInfo, std::vector<std::tuple<int32_t, std::string, Actor<Tiled2dMapVectorTile>>>> tiles;
    std::unordered_map<Tiled2dMapTileInfo, Tiled2dMapLayerMaskWrapper> tileMaskMap;
    std::unordered_set<Tiled2dMapTileInfo> tilesReady;
    std::unordered_map<Tiled2dMapTileInfo, std::unordered_set<int32_t>> tilesReadyControlSet;

    std::atomic_bool isPaused = true;
};
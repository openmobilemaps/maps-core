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
#include "Tiled2dMapVectorLayerConfig.h"
#include "VectorMapSourceDescription.h"
#include "RenderPassInterface.h"
#include "Tiled2dMapRasterTileInfo.h"
#include "Tiled2dMapVectorTileInfo.h"
#include "Tiled2dMapVectorLayerTileCallbackInterface.h"
#include "Tiled2dMapLayerMaskWrapper.h"
#include "Tiled2dMapVectorLayerSelectionCallbackInterface.h"
#include "Actor.h"
#include "Tiled2dMapVectorReadyManager.h"
#include <unordered_set>

class Tiled2dMapVectorLayer;
class Tiled2dMapVectorTile;
class SpriteData;
class Tiled2dMapVectorLayerUpdateInformation;

class Tiled2dMapVectorSourceDataManager : public ActorObject {
public:
    Tiled2dMapVectorSourceDataManager(const WeakActor<Tiled2dMapVectorLayer> &vectorLayer,
                                      const std::shared_ptr<VectorMapDescription> &mapDescription,
                                      const std::shared_ptr<Tiled2dMapVectorLayerConfig> &layerConfig,
                                      const std::string &source,
                                      const Actor<Tiled2dMapVectorReadyManager> &readyManager,
                                      const std::shared_ptr<Tiled2dMapVectorStateManager> &featureStateManager);

    virtual void onAdded(const std::weak_ptr<::MapInterface> &mapInterface);

    virtual void onRemoved();

    virtual void pause() = 0;

    virtual void resume() = 0;

    virtual void setAlpha(float alpha) = 0;

    virtual void setSelectionDelegate(const std::weak_ptr<Tiled2dMapVectorLayerSelectionCallbackInterface> &selectionDelegate);

    virtual void onRasterTilesUpdated(const std::string &layerName, VectorSet<Tiled2dMapRasterTileInfo> currentTileInfos) {};

    virtual void onVectorTilesUpdated(const std::string &sourceName, VectorSet<Tiled2dMapVectorTileInfo> currentTileInfos) {};

    virtual void updateLayerDescription(std::shared_ptr<VectorLayerDescription> layerDescription,
                                        int32_t legacyIndex,
                                        bool needsTileReplace) = 0;

    virtual void updateLayerDescriptions(std::vector<Tiled2dMapVectorLayerUpdateInformation> layerUpdates);

    virtual bool onClickUnconfirmed(const std::unordered_set<std::string> &layers, const Vec2F &posScreen) = 0;

    virtual bool onClickConfirmed(const std::unordered_set<std::string> &layers, const Vec2F &posScreen) = 0;

    virtual bool onDoubleClick(const std::unordered_set<std::string> &layers, const Vec2F &posScreen) = 0;

    virtual bool onLongPress(const std::unordered_set<std::string> &layers, const Vec2F &posScreen) = 0;

    virtual bool onTwoFingerClick(const std::unordered_set<std::string> &layers, const Vec2F &posScreen1, const Vec2F &posScreen2) = 0;

    virtual void clearTouch() = 0;

    virtual bool performClick(const std::unordered_set<std::string> &layers, const Coord &coord) = 0;

    virtual void setSprites(std::string spriteId, std::shared_ptr<SpriteData> spriteData, std::shared_ptr<TextureHolderInterface> spriteTexture) {}

    virtual LayerReadyState isReadyToRenderOffscreen();

protected:
    std::weak_ptr<MapInterface> mapInterface;
    const WeakActor<Tiled2dMapVectorLayer> vectorLayer;
    const std::shared_ptr<VectorMapDescription> mapDescription;
    std::shared_ptr<Tiled2dMapVectorLayerConfig> layerConfig;
    const std::string source;
    const size_t sourceHash;
    std::weak_ptr<Tiled2dMapVectorLayerSelectionCallbackInterface> selectionDelegate;

    std::unordered_map<std::string, int32_t> layerNameIndexMap;
    std::unordered_set<int32_t> modifyingMaskLayers;
    std::unordered_set<int32_t> selfMaskedLayers;

    std::optional<::RectI> scissorRect = std::nullopt;

    const Actor<Tiled2dMapVectorReadyManager> readyManager;
    size_t readyManagerIndex;

    const std::shared_ptr<Tiled2dMapVectorStateManager> featureStateManager;
};

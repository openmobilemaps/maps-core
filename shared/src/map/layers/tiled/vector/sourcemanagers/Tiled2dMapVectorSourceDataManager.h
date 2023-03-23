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
#include "Tiled2dMapVectorLayerTileCallbackInterface.h"
#include "Tiled2dMapLayerMaskWrapper.h"
#include "Actor.h"
#include <unordered_set>

class Tiled2dMapVectorLayer;
class Tiled2dMapVectorTile;

class Tiled2dMapVectorSourceDataManager : public ActorObject {
public:
    Tiled2dMapVectorSourceDataManager(const WeakActor<Tiled2dMapVectorLayer> &vectorLayer,
                                          const std::shared_ptr<VectorMapDescription> &mapDescription,
                                          const std::string &source);

    virtual void onAdded(const std::weak_ptr<::MapInterface> &mapInterface);

    virtual void onRemoved();

    virtual void pause() = 0;

    virtual void resume() = 0;

    virtual void setAlpha(float alpha) = 0;

    virtual void setScissorRect(const std::optional<RectI> &scissorRect);

    virtual void setSelectionDelegate(const WeakActor<Tiled2dMapVectorLayerSelectionInterface> &selectionDelegate);

    virtual void setSelectedFeatureIdentifier(std::optional<int64_t> identifier);

    virtual void onRasterTilesUpdated(const std::string &layerName, std::unordered_set<Tiled2dMapRasterTileInfo> currentTileInfos) {};

    virtual void onVectorTilesUpdated(const std::string &sourceName, std::unordered_set<Tiled2dMapVectorTileInfo> currentTileInfos) {};

    virtual void updateLayerDescription(std::shared_ptr<VectorLayerDescription> layerDescription) = 0;

    virtual bool onClickUnconfirmed(const std::unordered_set<std::string> &layers, const Vec2F &posScreen) = 0;

    virtual bool onClickConfirmed(const std::unordered_set<std::string> &layers, const Vec2F &posScreen) = 0;

    virtual bool onDoubleClick(const std::unordered_set<std::string> &layers, const Vec2F &posScreen) = 0;

    virtual bool onLongPress(const std::unordered_set<std::string> &layers, const Vec2F &posScreen) = 0;

    virtual bool onTwoFingerClick(const std::unordered_set<std::string> &layers, const Vec2F &posScreen1, const Vec2F &posScreen2) = 0;

    virtual void clearTouch() = 0;

protected:
    std::weak_ptr<MapInterface> mapInterface;
    const WeakActor<Tiled2dMapVectorLayer> vectorLayer;
    const std::shared_ptr<VectorMapDescription> mapDescription;
    const std::string source;
    WeakActor<Tiled2dMapVectorLayerSelectionInterface> selectionDelegate;

    std::unordered_map<std::string, int32_t> layerNameIndexMap;

    std::optional<::RectI> scissorRect = std::nullopt;
};
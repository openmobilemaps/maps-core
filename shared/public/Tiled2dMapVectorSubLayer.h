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


#include <mutex>
#include <unordered_map>
#include <vtzero/vector_tile.hpp>
#include "SimpleLayerInterface.h"
#include "Tiled2dMapVectorTileInfo.h"
#include "QuadMaskObject.h"
#include "VectorTileGeometryHandler.h"
#include "VectorLayerDescription.h"
#include "Tiled2dMapVectorLayerTileCallbackInterface.h"
#include "Textured2dLayerObject.h"
#include "Tiled2dMapVectorTileInfo.h"

class Tiled2dMapVectorSubLayer : public SimpleLayerInterface {
public:
    virtual void update() override;

    virtual std::vector<std::shared_ptr<RenderPassInterface>> buildRenderPasses() override;

    virtual std::vector<std::shared_ptr<RenderPassInterface>> buildRenderPasses(const std::unordered_set<Tiled2dMapTileInfo> &tiles);

    virtual void onAdded(const std::shared_ptr<MapInterface> &mapInterface, int32_t layerIndex) override;

    virtual void onRemoved() override;

    virtual void pause() override;

    virtual void resume() override;

    virtual void hide() override;

    virtual void show() override;

    virtual void setAlpha(float alpha) override;

    virtual float getAlpha() override;

    virtual void setMaskingObject(const std::shared_ptr<::MaskingObjectInterface> &maskingObject) override;

    virtual void updateTileData(const Tiled2dMapTileInfo &tileInfo, const std::shared_ptr<MaskingObjectInterface> &tileMask,
                   const std::vector<Tiled2dMapVectorTileInfo::FeatureTuple> &layerFeatures);

    virtual void updateTileMask(const Tiled2dMapTileInfo &tileInfo, const std::shared_ptr<MaskingObjectInterface> &tileMask);

    virtual void clearTileData(const Tiled2dMapTileInfo &tileInfo);

    virtual std::string getLayerDescriptionIdentifier() { return ""; };

    void setTilesReadyDelegate(const std::weak_ptr<Tiled2dMapVectorLayerTileCallbackInterface> readyDelegate);

    virtual void setSelectedFeatureIdentfier(std::optional<int64_t> identifier);

protected:
    void setupGraphicsObject(const std::shared_ptr<Textured2dLayerObject> &object, const std::shared_ptr<TextureHolderInterface> &texture);

protected:
    std::recursive_mutex maskMutex;
    std::unordered_map<Tiled2dMapTileInfo, std::shared_ptr<MaskingObjectInterface>> tileMaskMap;
    std::recursive_mutex tilesInSetupMutex;
    std::unordered_set<Tiled2dMapTileInfo> tilesInSetup;

    std::shared_ptr<MapInterface> mapInterface;

    std::unordered_map<Tiled2dMapTileInfo, std::vector<std::shared_ptr<RenderPassInterface>>> renderPasses;

    std::weak_ptr<Tiled2dMapVectorLayerTileCallbackInterface> readyDelegate;

    std::recursive_mutex selectedFeatureIdentifierMutex;
    std::optional<int64_t> selectedFeatureIdentifier;
    float alpha = 1.0;
};

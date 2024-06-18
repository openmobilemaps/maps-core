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

#include "Tiled2dMapVectorSubLayer.h"
#include "BackgroundVectorLayerDescription.h"
#include "ColorShaderInterface.h"
#include "RenderObject.h"
#include "Tiled2dMapVectorTileInfo.h"
#include "PolygonPatternGroupShaderInterface.h"
#include "SpriteData.h"
#include "TextureHolderInterface.h"
#include "PolygonGroup2dLayerObject.h"
#include "PolygonPatternGroup2dLayerObject.h"

class Tiled2dMapVectorBackgroundSubLayer : public Tiled2dMapVectorSubLayer, public std::enable_shared_from_this<Tiled2dMapVectorBackgroundSubLayer> {
public:
    Tiled2dMapVectorBackgroundSubLayer(const std::shared_ptr<BackgroundVectorLayerDescription> &description,
                                       const std::shared_ptr<Tiled2dMapVectorStateManager> &featureStateManager): description(description), featureStateManager(featureStateManager) {};

    ~Tiled2dMapVectorBackgroundSubLayer() {}

    virtual void update() override {};

    virtual void onAdded(const std::shared_ptr<MapInterface> &mapInterface, int32_t layerIndex) override;

    virtual void onRemoved() override;

    virtual void pause() override;

    virtual void resume() override;

    virtual void hide() override;

    virtual void show() override;

    virtual void setAlpha(float alpha) override;
    
    virtual std::vector<std::shared_ptr<RenderPassInterface>> buildRenderPasses() override;

    virtual std::vector<std::shared_ptr<RenderPassInterface>> buildRenderPasses(const std::unordered_set<Tiled2dMapTileInfo> &tiles) override;

    virtual void updateTileData(const Tiled2dMapTileInfo &tileInfo, const std::shared_ptr<MaskingObjectInterface> &tileMask,
                                const std::vector<Tiled2dMapVectorTileInfo::FeatureTuple> &layerFeatures) override {};

    void updateTileMask(const Tiled2dMapTileInfo &tileInfo, const std::shared_ptr<MaskingObjectInterface> &tileMask) override {};

    virtual void clearTileData(const Tiled2dMapTileInfo &tileInfo) override {};

    virtual std::string getLayerDescriptionIdentifier() override;

    void setSprites(std::shared_ptr<SpriteData> spriteData, std::shared_ptr<TextureHolderInterface> spriteTexture);

private:
    const static int32_t SUBDIVISION_FACTOR_3D_DEFAULT = 4;

    std::shared_ptr<BackgroundVectorLayerDescription> description;

    double dpFactor = 1.0;

    std::vector<std::shared_ptr<RenderPassInterface>> renderPasses;

    std::string patternName;
    std::shared_ptr<PolygonPatternGroup2dLayerObject> patternObject;
    std::shared_ptr<PolygonGroup2dLayerObject> polygonObject;

    const std::shared_ptr<Tiled2dMapVectorStateManager> featureStateManager;

    std::recursive_mutex mutex;

    std::shared_ptr<SpriteData> spriteData;
    std::shared_ptr<TextureHolderInterface> spriteTexture;
};

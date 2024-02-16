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

#include "Actor.h"
#include "Tiled2dMapVersionedTileInfo.h"
#include "Value.h"
#include "MapInterface.h"
#include "VectorTileGeometryHandler.h"
#include "RenderObjectInterface.h"
#include "Quad2dInterface.h"
#include "ColorShaderInterface.h"
#include "SimpleTouchInterface.h"
#include "VectorLayerDescription.h"
#include "Tiled2dMapVectorLayerTileCallbackInterface.h"
#include "Tiled2dMapVectorLayerSelectionCallbackInterface.h"
#include "SpriteData.h"
#include "Tiled2dMapVectorTileInfo.h"
#include "Tiled2dMapVectorLayerConfig.h"

typedef std::shared_ptr<TextureHolderInterface> Tiled2dMapVectorTileDataRaster;
typedef std::shared_ptr<std::vector<Tiled2dMapVectorTileInfo::FeatureTuple>> Tiled2dMapVectorTileDataVector;

class Tiled2dMapVectorTile : public ActorObject,
                             public SimpleTouchInterface {
public:
    Tiled2dMapVectorTile(const std::weak_ptr<MapInterface> &mapInterface,
                         const Tiled2dMapVersionedTileInfo &tileInfo,
                         const std::shared_ptr<VectorLayerDescription> &description,
                         const std::shared_ptr<Tiled2dMapVectorLayerConfig> &layerConfig,
                         const WeakActor<Tiled2dMapVectorLayerTileCallbackInterface> &tileReadyInterface,
                         const std::shared_ptr<Tiled2dMapVectorStateManager> &featureStateManager);

    virtual void updateVectorLayerDescription(const std::shared_ptr<VectorLayerDescription> &description,
                                        const Tiled2dMapVectorTileDataVector &layerData);

    virtual void updateRasterLayerDescription(const std::shared_ptr<VectorLayerDescription> &description,
                                         const Tiled2dMapVectorTileDataRaster &layerData);

    virtual void update() = 0;

    virtual std::vector<std::shared_ptr<RenderObjectInterface>> generateRenderObjects() = 0;

    virtual void clear() = 0;

    virtual void setup() = 0;

    virtual void setAlpha(float alpha);

    virtual float getAlpha();

    virtual void setVectorTileData(const Tiled2dMapVectorTileDataVector &tileData) {};

    virtual void setRasterTileData(const Tiled2dMapVectorTileDataRaster &tileData) {};

    virtual void setSpriteData(const std::shared_ptr<SpriteData> &spriteData, const std::shared_ptr<TextureHolderInterface> &spriteTexture) {};

    void setSelectionDelegate(const std::weak_ptr<Tiled2dMapVectorLayerSelectionCallbackInterface> &selectionDelegate);

    virtual bool performClick(const Coord &coord) = 0;

protected:
    const std::weak_ptr<MapInterface> mapInterface;
    const Tiled2dMapVersionedTileInfo tileInfo;
    std::shared_ptr<VectorLayerDescription> description;
    std::shared_ptr<Tiled2dMapVectorLayerConfig> layerConfig;
    const WeakActor<Tiled2dMapVectorLayerTileCallbackInterface> tileCallbackInterface;

    std::optional<float> lastAlpha = std::nullopt;
    float alpha = 1.0;
    double dpFactor = 1.0;

    std::weak_ptr<Tiled2dMapVectorLayerSelectionCallbackInterface> selectionDelegate;
    bool multiselect = false;

    const std::shared_ptr<Tiled2dMapVectorStateManager> featureStateManager;
};

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
#include "Tiled2dMapTileInfo.h"
#include "Value.h"
#include "MapInterface.h"
#include "VectorTileGeometryHandler.h"
#include "RenderPassInterface.h"
#include "Quad2dInterface.h"
#include "ColorShaderInterface.h"
#include "Tiled2dMapVectorLayerSelectionInterface.h"
#include "SimpleTouchInterface.h"
#include "VectorLayerDescription.h"
#include "Tiled2dMapVectorLayerReadyInterface.h"

typedef std::shared_ptr<TextureHolderInterface> Tiled2dMapVectorTileDataRaster;
typedef std::vector<std::tuple<const FeatureContext, const VectorTileGeometryHandler>> Tiled2dMapVectorTileDataVector;
typedef std::variant<Tiled2dMapVectorTileDataRaster, Tiled2dMapVectorTileDataVector> Tiled2dMapVectorTileDataVariant;

class Tiled2dMapVectorTile : public ActorObject,
                             public SimpleTouchInterface {
public:
    Tiled2dMapVectorTile(const std::weak_ptr<MapInterface> &mapInterface,
                         const Tiled2dMapTileInfo &tileInfo,
                         const std::shared_ptr<VectorLayerDescription> &description,
                         const WeakActor<Tiled2dMapVectorLayerReadyInterface> &tileReadyInterface);

    virtual void updateLayerDescription(const std::shared_ptr<VectorLayerDescription> &description,
                                        const Tiled2dMapVectorTileDataVariant &layerData);

    virtual void update() = 0;

    virtual std::vector<std::shared_ptr<RenderPassInterface>> buildRenderPasses() = 0;

    virtual void clear() = 0;

    virtual void setup() = 0;

    virtual void setAlpha(float alpha);

    virtual float getAlpha();

    virtual void setScissorRect(const std::optional<::RectI> &scissorRect) = 0;

    virtual void setTileData(const std::shared_ptr<MaskingObjectInterface> &tileMask,
                             const Tiled2dMapVectorTileDataVariant &tileData) = 0;

    virtual void updateTileMask(const std::shared_ptr<MaskingObjectInterface> &tileMask) = 0;

    void setSelectionDelegate(const WeakActor<Tiled2dMapVectorLayerSelectionInterface> &selectionDelegate);

    void setSelectedFeatureIdentifier(std::optional<int64_t> identifier);

protected:
    const std::weak_ptr<MapInterface> mapInterface;
    const Tiled2dMapTileInfo tileInfo;
    std::shared_ptr<VectorLayerDescription> description;
    const WeakActor<Tiled2dMapVectorLayerReadyInterface> tileReadyInterface;

    float alpha = 1.0;

    WeakActor<Tiled2dMapVectorLayerSelectionInterface> selectionDelegate;
    std::optional<int64_t> selectedFeatureIdentifier;
};

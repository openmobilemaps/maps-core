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

#include "Tiled2dMapVectorTile.h"
#include "BackgroundVectorLayerDescription.h"
#include "MapInterface.h"
#include "RenderObject.h"

class Tiled2dMapVectorBackgroundTile : public Tiled2dMapVectorTile, public std::enable_shared_from_this<Tiled2dMapVectorBackgroundTile> {
public:
    Tiled2dMapVectorBackgroundTile(const std::weak_ptr<MapInterface> &mapInterface,
                                const Tiled2dMapTileInfo &tileInfo,
                                const WeakActor<Tiled2dMapVectorLayer> &vectorLayer,
                                const std::shared_ptr<BackgroundVectorLayerDescription> &description);

    void updateLayerDescription(const std::shared_ptr<VectorLayerDescription> &description,
                                const std::vector<std::tuple<const FeatureContext, const VectorTileGeometryHandler>> &layerFeatures) override;

    void update() override;

    virtual std::vector<std::shared_ptr<::RenderObjectInterface>> getRenderObjects() override;

    virtual void clear() override;

    virtual void setup() override;

    virtual void setScissorRect(const std::optional<::RectI> &scissorRect) override;

    virtual void setTileData(const std::shared_ptr<MaskingObjectInterface> &tileMask,
                             const std::vector<std::tuple<const FeatureContext, const VectorTileGeometryHandler>> &layerFeatures) override;

    virtual void updateTileMask(const std::shared_ptr<MaskingObjectInterface> &tileMask) override;

private:
    std::shared_ptr<RenderObject> renderObject;

    std::shared_ptr<::ColorShaderInterface> shader;
};

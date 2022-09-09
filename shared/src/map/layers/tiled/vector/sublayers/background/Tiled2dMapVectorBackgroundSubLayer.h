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

class Tiled2dMapVectorBackgroundSubLayer : public Tiled2dMapVectorSubLayer, public std::enable_shared_from_this<Tiled2dMapVectorBackgroundSubLayer> {
public:
    Tiled2dMapVectorBackgroundSubLayer(const std::shared_ptr<BackgroundVectorLayerDescription> &description): description(description) {};

    ~Tiled2dMapVectorBackgroundSubLayer() {}

    virtual void update() override {};

    virtual void onAdded(const std::shared_ptr<MapInterface> &mapInterface) override;

    virtual void onRemoved() override;

    virtual void pause() override;

    virtual void resume() override;

    virtual void hide() override;

    virtual void show() override;

    virtual std::vector<std::shared_ptr<RenderPassInterface>> buildRenderPasses() override;

    virtual std::vector<std::shared_ptr<RenderPassInterface>> buildRenderPasses(const std::unordered_set<Tiled2dMapTileInfo> &tiles) override;

    virtual void updateTileData(const Tiled2dMapTileInfo &tileInfo, const std::shared_ptr<MaskingObjectInterface> &tileMask,
                                const std::vector<std::tuple<const FeatureContext, const VectorTileGeometryHandler>> &layerFeatures) override {};

    void updateTileMask(const Tiled2dMapTileInfo &tileInfo, const std::shared_ptr<MaskingObjectInterface> &tileMask) override {};

    virtual void clearTileData(const Tiled2dMapTileInfo &tileInfo) override {};

    virtual void setScissorRect(const std::optional<::RectI> &scissorRect) override;

private:
    std::shared_ptr<BackgroundVectorLayerDescription> description;

    std::shared_ptr<Quad2dInterface> object;

    std::vector<std::shared_ptr<RenderPassInterface>> renderPasses;
};

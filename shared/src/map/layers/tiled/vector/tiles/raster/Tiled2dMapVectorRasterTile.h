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
#include "RasterVectorLayerDescription.h"
#include "Textured2dLayerObject.h"

class Tiled2dMapVectorRasterTile : public Tiled2dMapVectorTile, public std::enable_shared_from_this<Tiled2dMapVectorRasterTile> {
public:
    Tiled2dMapVectorRasterTile(const std::weak_ptr<MapInterface> &mapInterface,
                                const Tiled2dMapTileInfo &tileInfo,
                                const WeakActor<Tiled2dMapVectorLayerReadyInterface> &tileReadyInterface,
                                const std::shared_ptr<RasterVectorLayerDescription> &description);

    void updateLayerDescription(const std::shared_ptr<VectorLayerDescription> &description,
                                const Tiled2dMapVectorTileDataVariant &tileData) override;

    void update() override;

    virtual std::vector<std::shared_ptr<RenderObjectInterface>> generateRenderObjects() override;

    void clear() override;

    void setup() override;

    void setAlpha(float alpha) override;

    float getAlpha() override;

    void setTileData(const Tiled2dMapVectorTileDataVariant &tileData) override;

    void setupTile(const Tiled2dMapVectorTileDataRaster tileData);

private:
    std::shared_ptr<Textured2dLayerObject> tileObject;
    std::shared_ptr<TextureHolderInterface> tileData;
};

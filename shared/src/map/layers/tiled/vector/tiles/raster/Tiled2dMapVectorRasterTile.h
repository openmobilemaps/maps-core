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
#include "RectD.h"
#include <unordered_map>

class Tiled2dMapVectorRasterTile : public Tiled2dMapVectorTile, public std::enable_shared_from_this<Tiled2dMapVectorRasterTile> {
public:
    Tiled2dMapVectorRasterTile(const std::weak_ptr<MapInterface> &mapInterface,
                               const std::weak_ptr<Tiled2dMapVectorLayer> &vectorLayer,
                               const Tiled2dMapVersionedTileInfo &tileInfo,
                               const WeakActor<Tiled2dMapVectorLayerTileCallbackInterface> &tileCallbackInterface,
                               const std::shared_ptr<RasterVectorLayerDescription> &description,
                               const std::shared_ptr<Tiled2dMapVectorLayerConfig> &layerConfig,
                               const std::shared_ptr<Tiled2dMapVectorStateManager> &featureStateManager);

    void updateRasterLayerDescription(const std::shared_ptr<VectorLayerDescription> &description,
                                      const Tiled2dMapVectorTileDataRaster &tileData) override;

    bool update() override;

    virtual std::vector<std::shared_ptr<RenderObjectInterface>> generateRenderObjects() override;

    void clear() override;
    void setup() override;

    void pause() override;
    void resume() override;

    void setAlpha(float alpha) override;

    float getAlpha() override;

    virtual void setRasterTileData(const Tiled2dMapVectorTileDataRaster &tileData) override;

    void setSpriteData(const std::string &spriteId, const std::shared_ptr<SpriteData> &spriteData, const std::shared_ptr<TextureHolderInterface> &spriteTexture) override;

    void setupTile(const Tiled2dMapVectorTileDataRaster tileData);

    bool performClick(const Coord &coord) override;

private:
    std::optional<std::pair<std::shared_ptr<TextureHolderInterface>, RectD>> resolveLookupTexture();
    std::optional<RasterShaderStyle> getTextureLookupStyle(const RasterShaderStyle &baseStyle, double zoomIdentifier);

    std::shared_ptr<Textured2dLayerObject> tileObject;
    std::shared_ptr<TextureHolderInterface> tileData;
    std::unordered_map<std::string, std::pair<std::shared_ptr<SpriteData>, std::shared_ptr<TextureHolderInterface>>> sprites;

    UsedKeysCollection usedKeys;
    bool isStyleZoomDependant = true;
    bool isStyleStateDependant = true;
    std::optional<double> lastZoom = std::nullopt;
    bool isVisible = true;

    std::optional<RasterShaderStyle> lastStyle;

    Tiled2dMapZoomInfo zoomInfo;

    int32_t subdivisionFactor = 3;
};

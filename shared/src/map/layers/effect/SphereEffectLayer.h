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
#include "SphereEffectLayerInterface.h"
#include "MapCamera3dInterface.h"

class SphereEffectLayer : public SimpleLayerInterface, public SphereEffectLayerInterface, public std::enable_shared_from_this<SphereEffectLayer> {
public:
    SphereEffectLayer() {};

    ~SphereEffectLayer() {}

    virtual void update() override;

    virtual void onAdded(const std::shared_ptr<MapInterface> &mapInterface, int32_t layerIndex) override;

    virtual void onRemoved() override;

    virtual void pause() override;

    virtual void resume() override;

    virtual void hide() override;

    virtual void show() override;

    virtual void setAlpha(float alpha) override;
    
    virtual std::vector<std::shared_ptr<RenderPassInterface>> buildRenderPasses() override;

    void setSprites(std::shared_ptr<SpriteData> spriteData, std::shared_ptr<TextureHolderInterface> spriteTexture);

    std::shared_ptr<::LayerInterface> asLayerInterface() override;

private:

    double alpha = 1.0;

    std::vector<std::shared_ptr<RenderPassInterface>> renderPasses;

    std::recursive_mutex mutex;

    std::shared_ptr<MapInterface> mapInterface;
    std::shared_ptr<MapCamera3dInterface> camera;

    std::shared_ptr<SphereEffectShaderInterface> shader;
    std::shared_ptr<::Quad2dInterface> quad;
};

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

#include "LayerInterface.h"
#include "Textured2dLayerObject.h"
#include "Tiled2dMapLayer.h"
#include "Tiled2dMapRasterLayerInterface.h"
#include "Tiled2dMapRasterSource.h"
#include "Tiled2dMapRasterLayerCallbackInterface.h"
#include <mutex>
#include <unordered_map>

class Tiled2dMapRasterLayer
        : public Tiled2dMapLayer, public Tiled2dMapRasterLayerInterface {
public:
    Tiled2dMapRasterLayer(const std::shared_ptr<::Tiled2dMapLayerConfig> &layerConfig,
                          const std::shared_ptr<::TextureLoaderInterface> &textureLoader);

    virtual void onAdded(const std::shared_ptr<::MapInterface> &mapInterface) override;

    virtual void onRemoved() override;

    virtual std::shared_ptr<::LayerInterface> asLayerInterface() override;

    virtual void update() override;

    virtual std::vector<std::shared_ptr<::RenderPassInterface>> buildRenderPasses() override;

    virtual void pause() override;

    virtual void resume() override;

    virtual void onTilesUpdated() override;

    virtual void setCallbackHandler(const std::shared_ptr<Tiled2dMapRasterLayerCallbackInterface> &handler) override;

    virtual std::shared_ptr<Tiled2dMapRasterLayerCallbackInterface> getCallbackHandler() override;

    virtual void removeCallbackHandler() override;

    virtual void setAlpha(double alpha) override;

    virtual double getAlpha() override;

    bool onClickConfirmed(const Vec2F &posScreen) override;

    bool onLongPress(const Vec2F &posScreen) override;

private:
    std::shared_ptr<TextureLoaderInterface> textureLoader;
    std::shared_ptr<Tiled2dMapRasterSource> rasterSource;

    std::recursive_mutex updateMutex;
    std::unordered_map<Tiled2dMapRasterTileInfo, std::shared_ptr<Textured2dLayerObject>> tileObjectMap;
    std::vector<std::shared_ptr<RenderPassInterface>> renderPasses;

    std::shared_ptr<Tiled2dMapRasterLayerCallbackInterface> callbackHandler;

    double alpha;
};

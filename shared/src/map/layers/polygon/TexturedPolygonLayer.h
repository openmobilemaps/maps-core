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

#include "AlphaShaderInterface.h"
#include "PolygonCoord.h"
#include "RectCoord.h"
#include "SimpleLayerInterface.h"
#include "TextureHolderInterface.h"
#include "Textured2dLayerObject.h"
#include "TexturedPolygonInterface.h"
#include "TexturedPolygonLayerInterface.h"
#include <atomic>
#include <memory>
#include <mutex>
#include <optional>

class TexturedPolygonLayer : public TexturedPolygonLayerInterface,
                             public SimpleLayerInterface,
                             public std::enable_shared_from_this<TexturedPolygonLayer> {
  public:
    TexturedPolygonLayer();

    // TexturedPolygonLayerInterface
    void setPolygon(const ::PolygonCoord &polygon, const ::RectCoord &textureBounds) override;

    void loadTexture(const std::shared_ptr<::TextureHolderInterface> &texture) override;

    void setAlpha(float alpha) override;

    void setRenderPassIndex(int32_t index) override;

    std::shared_ptr<::LayerInterface> asLayerInterface() override;

    // SimpleLayerInterface / LayerInterface
    void update() override;

    std::vector<std::shared_ptr<::RenderPassInterface>> buildRenderPasses() override;

    void onAdded(const std::shared_ptr<MapInterface> &mapInterface, int32_t layerIndex) override;

    void onRemoved() override;

    void pause() override;

    void resume() override;

    void hide() override;

    void show() override;

    float getAlpha() override;

  private:
    void setupGraphics();
    void applyPolygon();
    void applyTexture();
    void rebuildRenderPasses();

    std::shared_ptr<MapInterface> mapInterface;

    std::shared_ptr<AlphaShaderInterface> shader;
    std::shared_ptr<::TexturedPolygonInterface> texturedPolygon;
    std::shared_ptr<Textured2dLayerObject> layerObject;

    std::recursive_mutex stateMutex;
    std::optional<::PolygonCoord> pendingPolygon;
    std::optional<::RectCoord> pendingTextureBounds;
    std::shared_ptr<::TextureHolderInterface> pendingTexture;
    bool polygonApplied = false;
    bool textureApplied = false;

    std::vector<std::shared_ptr<::RenderPassInterface>> renderPasses;
    std::recursive_mutex renderPassMutex;

    std::atomic<bool> isHidden = false;
    std::atomic<float> alpha = 1.0f;
    std::atomic<int32_t> renderPassIndex = 0;
};

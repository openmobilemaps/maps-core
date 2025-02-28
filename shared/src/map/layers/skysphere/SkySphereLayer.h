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
#include "MapCameraInterface.h"
#include "Quad2dInterface.h"
#include "SimpleLayerInterface.h"
#include "SkySphereLayerInterface.h"
#include "SkySphereShaderInterface.h"

class SkySphereLayer
        : public ActorObject,
          public SimpleLayerInterface,
          public SkySphereLayerInterface,
          public std::enable_shared_from_this<SkySphereLayer> {
public:
    // SimpleLayerInterface

    void update() override;

    std::vector<std::shared_ptr<::RenderPassInterface>> buildRenderPasses() override;

    void onAdded(const std::shared_ptr<MapInterface> &mapInterface, int32_t layerIndex) override;

    void onRemoved() override;

    void pause() override;

    void resume() override;

    void hide() override;

    void show() override;

    // SkySphereLayerInterface

    std::shared_ptr<::LayerInterface> asLayerInterface() override;

    void setTexture(const std::shared_ptr<::TextureHolderInterface> &texture) override;

private:
    void setupSkySphere();

    std::shared_ptr<MapInterface> mapInterface;

    std::shared_ptr<SkySphereShaderInterface> shader;
    std::shared_ptr<::Quad2dInterface> quad;

    std::vector<std::shared_ptr<::RenderPassInterface>> renderPasses;
    bool isHidden = false;

    std::shared_ptr<::TextureHolderInterface> skySphereTexture;
};


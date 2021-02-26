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

#include <unordered_map>
#include "GraphicsObjectInterface.h"
#include "RenderPassInterface.h"

class RenderPass : public RenderPassInterface {
public:
    RenderPass(RenderPassConfig config, std::vector<std::shared_ptr<::GraphicsObjectInterface>> graphicsObjects);

    RenderPass(RenderPassConfig config, std::vector<std::shared_ptr<::GraphicsObjectInterface>> graphicsObjects,
               std::unordered_map<int, std::vector<float>> customObjectTransforms);

    virtual std::vector<std::shared_ptr<::GraphicsObjectInterface>> getGraphicsObjects() override;

    virtual std::unordered_map<int32_t, std::vector<float>> getCustomObjectTransforms() override;

    virtual void setCustomObjectTransforms(const std::unordered_map<int32_t, std::vector<float>> & customObjectTransforms) override;

    virtual RenderPassConfig getRenderPassConfig() override;

private:
    RenderPassConfig config;
    std::vector<std::shared_ptr<::GraphicsObjectInterface>> graphicsObjects;
    std::unordered_map<int, std::vector<float>> customObjectTransforms;
};

/*
 * Copyright (c) 2021 Ubique Innovation AG <https://www.ubique.ch>
 *
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this
 *  file, You can obtain one at https://mozilla.org/MPL/2.0/.
 *
 *  SPDX-License-Identifier: MPL-2.0
 */

#include "RenderPass.h"

RenderPass::RenderPass(RenderPassConfig config, std::vector<std::shared_ptr<::GraphicsObjectInterface>> graphicsObjects)
        : config(config), graphicsObjects(graphicsObjects),
          customObjectTransforms(std::unordered_map<int, std::vector<float>>()) { }

RenderPass::RenderPass(RenderPassConfig config, std::vector<std::shared_ptr<::GraphicsObjectInterface>> graphicsObjects,
                       std::unordered_map<int, std::vector<float>> customObjectTransforms)
        : config(config), graphicsObjects(graphicsObjects), customObjectTransforms(customObjectTransforms) {}

std::vector<std::shared_ptr<::GraphicsObjectInterface>> RenderPass::getGraphicsObjects() { return graphicsObjects; }

std::unordered_map<int32_t, std::vector<float>> RenderPass::getCustomObjectTransforms() {
    return customObjectTransforms;
}

void RenderPass::setCustomObjectTransforms(const std::unordered_map<int32_t, std::vector<float>> &customObjectTransforms) {
    this->customObjectTransforms = customObjectTransforms;
}


RenderPassConfig RenderPass::getRenderPassConfig() { return config; }

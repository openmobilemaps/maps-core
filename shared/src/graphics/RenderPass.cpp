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

RenderPass::RenderPass(RenderPassConfig config, std::vector<std::shared_ptr<::RenderObjectInterface>> renderObjects)
        : config(config), renderObjects(renderObjects) {}

RenderPass::RenderPass(RenderPassConfig config, std::vector<std::shared_ptr<::RenderObjectInterface>> renderObjects, std::shared_ptr<GraphicsObjectInterface> maskingObject)
        : config(config), renderObjects(renderObjects), maskingObject(maskingObject) {}

std::vector<std::shared_ptr<::RenderObjectInterface>> RenderPass::getRenderObjects() { return renderObjects; }

void RenderPass::addRenderObject(const std::shared_ptr<RenderObjectInterface> &renderObject) {
    renderObjects.push_back(renderObject);
}

RenderPassConfig RenderPass::getRenderPassConfig() { return config; }

std::shared_ptr<::GraphicsObjectInterface> RenderPass::getMaskingObject() {
    return maskingObject;
}
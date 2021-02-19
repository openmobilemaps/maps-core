/*
 * Copyright (c) 2021 Ubique Innovation AG <https://www.ubique.ch>
 *
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this
 *  file, You can obtain one at https://mozilla.org/MPL/2.0/.
 *
 *  SPDX-License-Identifier: MPL-2.0
 */

#include "Renderer.h"
#include "CameraInterface.h"
#include "RenderPassInterface.h"

void Renderer::addToRenderQueue(const std::shared_ptr<RenderPassInterface> &renderPass) { renderQueue.push(renderPass); }

/** Ensure calling on graphics thread */
void Renderer::drawFrame(const std::shared_ptr<RenderingContextInterface> &renderingContext,
                         const std::shared_ptr<CameraInterface> &camera) {

    auto mvpMatrix = camera->getMvpMatrix();
    auto mvpMatrixPointer = (int64_t)mvpMatrix.data();

    renderingContext->setupDrawFrame();

    while (!renderQueue.empty()) {
        auto pass = renderQueue.front();

        for (const auto &object : pass->getGraphicsObjects()) {
            object->render(renderingContext, pass->getRenderPassConfig(), mvpMatrixPointer);
        }

        renderQueue.pop();
    }
}

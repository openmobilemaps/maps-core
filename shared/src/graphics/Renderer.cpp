/*
 * Copyright (c) 2021 Ubique Innovation AG <https://www.ubique.ch>
 *
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this
 *  file, You can obtain one at https://mozilla.org/MPL/2.0/.
 *
 *  SPDX-License-Identifier: MPL-2.0
 */

#include <Logger.h>
#include "Renderer.h"
#include "Matrix.h"
#include "CameraInterface.h"
#include "RenderPassInterface.h"
#include "RenderObjectInterface.h"

void Renderer::addToRenderQueue(const std::shared_ptr<RenderPassInterface> &renderPass) {
    renderQueue.push(renderPass);
}

/** Ensure calling on graphics thread */
void Renderer::drawFrame(const std::shared_ptr<RenderingContextInterface> &renderingContext,
                         const std::shared_ptr<CameraInterface> &camera) {

    auto vpMatrix = camera->getVpMatrix();
    auto vpMatrixPointer = (int64_t) vpMatrix.data();

    renderingContext->setupDrawFrame();

    while (!renderQueue.empty()) {
        auto pass = renderQueue.front();
        const auto &maskObject = pass->getMaskingObject();
        const bool hasMask = maskObject != nullptr;

        double factor = camera->getScalingFactor();
        const auto &renderObjects = pass->getRenderObjects();
        std::vector<float> tempMvpMatrix(16, 0);
        for (const auto &renderObject : renderObjects) {
            if (maskObject) {
                renderingContext->preRenderStencilMask();
                maskObject->renderAsMask(renderingContext, pass->getRenderPassConfig(), vpMatrixPointer, factor);
            }

            const auto &graphicsObject = renderObject->getGraphicsObject();
            if (renderObject->hasCustomModelMatrix()) {
                Matrix::multiplyMMC(tempMvpMatrix, 0, vpMatrix, 0, renderObject->getCustomModelMatrix(), 0);
                graphicsObject->render(renderingContext, pass->getRenderPassConfig(),
                               (int64_t) tempMvpMatrix.data(), hasMask, factor);
            } else {
                graphicsObject->render(renderingContext, pass->getRenderPassConfig(), vpMatrixPointer, hasMask, factor);
            }

            if (maskObject) {
                renderingContext->postRenderStencilMask();
            }
        }

        renderQueue.pop();
    }
}

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
#include "Matrix.h"
#include "RenderObjectInterface.h"
#include <Logger.h>

void Renderer::addToRenderQueue(const std::shared_ptr<RenderPassInterface> &renderPass) {
    int32_t renderPassIndex = renderPass->getRenderPassConfig().renderPassIndex;
    renderQueue[renderPassIndex].push_back(renderPass);
}

/** Ensure calling on graphics thread */
void Renderer::drawFrame(const std::shared_ptr<RenderingContextInterface> &renderingContext,
                         const std::shared_ptr<CameraInterface> &camera) {

    auto vpMatrix = camera->getVpMatrix();
    auto vpMatrixPointer = (int64_t)vpMatrix.data();

    for (const auto &[index, passes] : renderQueue) {
        for (const auto &pass : passes) {
            const auto &maskObject = pass->getMaskingObject();
            const bool hasMask = maskObject != nullptr;

            double factor = camera->getScalingFactor();
            const auto &renderObjects = pass->getRenderObjects();

            const auto &config = pass->getRenderPassConfig();

            renderingContext->setupDrawFrame(config);

            renderingContext->applyScissorRect(pass->getScissoringRect());

            if (hasMask) {
                renderingContext->preRenderStencilMask(config);
                maskObject->renderAsMask(renderingContext, config, vpMatrixPointer, factor);
                //LogDebug << "Has mask and mask is: " <<= (maskObject->asGraphicsObject()->isReady() ? "ready" : "not ready");
            }

            for (const auto &renderObject : renderObjects) {
                const auto &graphicsObject = renderObject->getGraphicsObject();
                if (renderObject->isScreenSpaceCoords()) {
                    graphicsObject->render(renderingContext, config, (int64_t) identityMatrix.data(), hasMask, factor);
                } else if (renderObject->hasCustomModelMatrix()) {
                    Matrix::multiplyMMC(tempMvpMatrix, 0, vpMatrix, 0, renderObject->getCustomModelMatrix(), 0);
                    graphicsObject->render(renderingContext, config, (int64_t)tempMvpMatrix.data(), hasMask,
                                           factor);
                } else {
                    graphicsObject->render(renderingContext, config, vpMatrixPointer, hasMask, factor);
                }
            }

            if (hasMask) {
                renderingContext->postRenderStencilMask(config);
            }

            renderingContext->endDrawFrame();
        }
    }
    renderQueue.clear();
}

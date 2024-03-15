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

    const auto vpMatrix = camera->getVpMatrix();
    const auto vpMatrixPointer = (int64_t)vpMatrix.data();

    const auto identityMatrixPointer = (int64_t) identityMatrix.data();

    renderingContext->setupDrawFrame();

    for (const auto &[index, passes] : renderQueue) {
        for (const auto &pass : passes) {
            const auto &maskObject = pass->getMaskingObject();
            const bool hasMask = maskObject != nullptr;
            const bool usesStencil = hasMask || pass->getRenderPassConfig().isPassMasked;

            double factor = camera->getScalingFactor();
            const auto &renderObjects = pass->getRenderObjects();

            auto scissoringRect = pass->getScissoringRect();
            if (scissoringRect) {
                renderingContext->applyScissorRect(scissoringRect);
            }

            if (usesStencil) {
                renderingContext->preRenderStencilMask();
            }

            if (hasMask) {
                maskObject->renderAsMask(renderingContext, pass->getRenderPassConfig(), vpMatrixPointer, identityMatrixPointer, factor);
            }

            for (const auto &renderObject : renderObjects) {
                const auto &graphicsObject = renderObject->getGraphicsObject();
                if (renderObject->isScreenSpaceCoords()) {
                    graphicsObject->render(renderingContext, pass->getRenderPassConfig(), identityMatrixPointer, identityMatrixPointer, hasMask, factor);
                } else if (renderObject->hasCustomModelMatrix()) {
                    const auto mMatrix = renderObject->getCustomModelMatrix();
                    const auto mMatrixPointer = (int64_t)mMatrix.data();
                    graphicsObject->render(renderingContext, pass->getRenderPassConfig(), vpMatrixPointer, mMatrixPointer, hasMask,
                                           factor);
                } else {
                    graphicsObject->render(renderingContext, pass->getRenderPassConfig(), vpMatrixPointer, identityMatrixPointer, hasMask, factor);
                }
            }

            if (usesStencil) {
                renderingContext->postRenderStencilMask();
            }

            if (scissoringRect) {
                renderingContext->applyScissorRect(std::nullopt);
            }
        }
    }
    renderQueue.clear();
}

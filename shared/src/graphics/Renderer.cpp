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
#include "ComputeObjectInterface.h"
#include <Logger.h>

void Renderer::addToRenderQueue(const std::shared_ptr<RenderPassInterface> &renderPass) {
    int32_t renderPassIndex = renderPass->getRenderPassConfig().renderPassIndex;
    renderQueue[renderPassIndex].push_back(renderPass);
}


void Renderer::addToComputeQueue(const std::shared_ptr<ComputePassInterface> &computePass) {
    computeQueue.push_back(computePass);
}

/** Ensure calling on graphics thread */
void Renderer::drawFrame(const std::shared_ptr<RenderingContextInterface> &renderingContext,
                         const std::shared_ptr<CameraInterface> &camera) {

    auto vpMatrix = camera->getVpMatrix();
    auto vpMatrixPointer = (int64_t)vpMatrix.data();

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
                maskObject->renderAsMask(renderingContext, pass->getRenderPassConfig(), vpMatrixPointer, factor);
            }

            for (const auto &renderObject : renderObjects) {
                const auto &graphicsObject = renderObject->getGraphicsObject();
                if (renderObject->isScreenSpaceCoords()) {
                    graphicsObject->render(renderingContext, pass->getRenderPassConfig(), (int64_t) identityMatrix.data(), hasMask, factor);
                } else if (renderObject->hasCustomModelMatrix()) {
                    Matrix::multiplyMMC(tempMvpMatrix, 0, vpMatrix, 0, renderObject->getCustomModelMatrix(), 0);
                    graphicsObject->render(renderingContext, pass->getRenderPassConfig(), (int64_t)tempMvpMatrix.data(), hasMask,
                                           factor);
                } else {
                    graphicsObject->render(renderingContext, pass->getRenderPassConfig(), vpMatrixPointer, hasMask, factor);
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

/** Ensure calling on graphics thread */
void Renderer::compute(const std::shared_ptr<RenderingContextInterface> &renderingContext,
                       const std::shared_ptr<CameraInterface> &camera) {
    double factor = camera->getScalingFactor();

    for (const auto &pass: computeQueue) {
        for (const auto &computeObject : pass->getComputeObjects())
            computeObject->compute(renderingContext, factor);
    }
    computeQueue.clear();
}
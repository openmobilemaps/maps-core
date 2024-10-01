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

    const auto viewMatrix = camera->getViewMatrix();
    const auto viewMatrixPointer = (int64_t)viewMatrix.data();
    const auto projectionMatrix = camera->getProjectionMatrix();
    const auto projectionMatrixPointer = (int64_t)projectionMatrix.data();

    const double factor = camera->getScalingFactor();

    const auto identityMatrixPointer = (int64_t) identityMatrix.data();

    renderingContext->setupDrawFrame();

    for (const auto &[index, passes] : renderQueue) {
        for (const auto &pass : passes) {
            const auto &maskObject = pass->getMaskingObject();
            const bool hasMask = maskObject != nullptr;
            const bool usesStencil = hasMask || pass->getRenderPassConfig().isPassMasked;

            const auto &renderObjects = pass->getRenderObjects();

            auto scissoringRect = pass->getScissoringRect();
            if (scissoringRect) {
                renderingContext->applyScissorRect(scissoringRect);
            }

            if (usesStencil) {
                renderingContext->preRenderStencilMask();
            }

            if (hasMask) {
                maskObject->renderAsMask(renderingContext,
                                         pass->getRenderPassConfig(),
                                         viewMatrixPointer,
                                         projectionMatrixPointer,
                                         identityMatrixPointer,
                                         factor);
            }

            for (const auto &renderObject : renderObjects) {
                const auto &graphicsObject = renderObject->getGraphicsObject();
                if (renderObject->isScreenSpaceCoords()) {
                    graphicsObject->render(renderingContext,
                                           pass->getRenderPassConfig(),
                                           identityMatrixPointer,
                                           identityMatrixPointer,
                                           identityMatrixPointer,
                                           hasMask,
                                           factor);
                } else if (renderObject->hasCustomModelMatrix()) {
                    const auto mMatrix = renderObject->getCustomModelMatrix();
                    const auto mMatrixPointer = (int64_t)mMatrix.data();
                    graphicsObject->render(renderingContext,
                                           pass->getRenderPassConfig(),
                                           viewMatrixPointer,
                                           projectionMatrixPointer,
                                           mMatrixPointer,
                                           hasMask,
                                           factor);
                } else {
                    graphicsObject->render(renderingContext,
                                           pass->getRenderPassConfig(),
                                           viewMatrixPointer,
                                           projectionMatrixPointer,
                                           identityMatrixPointer,
                                           hasMask,
                                           factor);
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

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
                         const std::shared_ptr<CameraInterface> &camera, const /*nullable*/ std::shared_ptr<RenderTargetInterface> & target) {

    const auto vpMatrix = camera->getVpMatrix();
    const auto vpMatrixPointer = (int64_t)vpMatrix.data();
	const auto origin = camera->getOrigin();
    const auto zeroOrigin = Vec3D(0, 0, 0);

    const double factor = camera->getScalingFactor();

    const auto identityMatrixPointer = (int64_t) identityMatrix.data();

    renderingContext->setupDrawFrame(vpMatrixPointer, origin, factor);

    for (const auto &[index, passes] : renderQueue) {
        for (const auto &pass : passes) {
            if (pass->getRenderPassConfig().renderTarget != target) {
                continue;
            }
            const auto &maskObject = pass->getMaskingObject();
            const bool hasMask = maskObject != nullptr;
            const bool usesStencil = hasMask || pass->getRenderPassConfig().isPassMasked;

            const auto &renderObjects = pass->getRenderObjects();

            bool prepared = false;

            auto scissoringRect = pass->getScissoringRect();

            for (const auto &renderObject : renderObjects) {

                if (renderObject->isHidden()) {
                    continue;
                }

                if (!prepared) {
                    if (scissoringRect) {
                        renderingContext->applyScissorRect(scissoringRect);
                    }

                    if (usesStencil) {
                        renderingContext->preRenderStencilMask();
                    }

                    if (hasMask) {
                        maskObject->renderAsMask(renderingContext, pass->getRenderPassConfig(), vpMatrixPointer, identityMatrixPointer, origin, factor);
                    }

                    prepared = true;
                }

                const auto &graphicsObject = renderObject->getGraphicsObject();
                if (renderObject->isScreenSpaceCoords()) {
                    graphicsObject->render(renderingContext, pass->getRenderPassConfig(), identityMatrixPointer, identityMatrixPointer, zeroOrigin, hasMask, factor);
                } else if (renderObject->hasCustomModelMatrix()) {
                    const auto mMatrix = renderObject->getCustomModelMatrix();
                    const auto mMatrixPointer = (int64_t)mMatrix.data();
                    graphicsObject->render(renderingContext, pass->getRenderPassConfig(), vpMatrixPointer, mMatrixPointer, origin, hasMask,
                                           factor);
                } else {
                    graphicsObject->render(renderingContext, pass->getRenderPassConfig(), vpMatrixPointer, identityMatrixPointer, origin, hasMask, factor);
                }
            }

            if (prepared) {
                if (usesStencil) {
                    renderingContext->postRenderStencilMask();
                }

                if (scissoringRect) {
                    renderingContext->applyScissorRect(std::nullopt);
                }
            }
        }
    }
    if (!target) {
        renderQueue.clear();
    }
}

/** Ensure calling on graphics thread */
void Renderer::compute(const std::shared_ptr<RenderingContextInterface> &renderingContext,
                       const std::shared_ptr<CameraInterface> &camera) {
    double factor = camera->getScalingFactor();
    const auto vpMatrix = camera->getVpMatrix();
    const auto vpMatrixPointer = (int64_t)vpMatrix.data();
    const auto origin = camera->getOrigin();

    for (const auto &pass: computeQueue) {
        for (const auto &computeObject : pass->getComputeObjects())
            computeObject->compute(renderingContext, vpMatrixPointer, origin, factor);
    }
    computeQueue.clear();
}

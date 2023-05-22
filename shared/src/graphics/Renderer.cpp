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
#include "OpenGlHelper.h"
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

    renderingContext->setupDrawFrame();

    for (const auto &[index, passes] : renderQueue) {
        for (const auto &pass : passes) {
            const auto &maskObject = pass->getMaskingObject();
            const bool hasMask = maskObject != nullptr;

            double factor = camera->getScalingFactor();
            const auto &renderObjects = pass->getRenderObjects();

            renderingContext->applyScissorRect(pass->getScissoringRect());
            OpenGlHelper::checkGlError("UBCM: CHECKCHECK 0");

            if (hasMask) {
                renderingContext->preRenderStencilMask();
                maskObject->renderAsMask(renderingContext, pass->getRenderPassConfig(), vpMatrixPointer, factor);
                OpenGlHelper::checkGlError("UBCM: CHECKCHECK MASK");
            }

            for (const auto &renderObject : renderObjects) {
                OpenGlHelper::checkGlError("UBCM: CHECKCHECK 1");
                const auto &graphicsObject = renderObject->getGraphicsObject();
                if (renderObject->isScreenSpaceCoords()) {
                    graphicsObject->render(renderingContext, pass->getRenderPassConfig(), (int64_t) identityMatrix.data(), hasMask, factor);
                } else if (renderObject->hasCustomModelMatrix()) {
                    std::stringstream ss1;
                    ss1 << "UBCM: CHECKCHECK " << graphicsObject.get();
                    OpenGlHelper::checkGlError(ss1.str());
                    Matrix::multiplyMMC(tempMvpMatrix, 0, vpMatrix, 0, renderObject->getCustomModelMatrix(), 0);
                    graphicsObject->render(renderingContext, pass->getRenderPassConfig(), (int64_t)tempMvpMatrix.data(), hasMask,
                                           factor);
                } else {
                    std::stringstream ss1;
                    ss1 << "UBCM: CHECKCHECK " << graphicsObject.get();
                    OpenGlHelper::checkGlError(ss1.str());
                    graphicsObject->render(renderingContext, pass->getRenderPassConfig(), vpMatrixPointer, hasMask, factor);
                    std::stringstream ss;
                    ss << "UBCM: CHECKCHECK END " << graphicsObject.get();
                    OpenGlHelper::checkGlError(ss.str());
                }
            }

            if (hasMask) {
                renderingContext->postRenderStencilMask();
            }
        }
    }
    renderQueue.clear();
}

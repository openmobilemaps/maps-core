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
#include "RenderPass.h"
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

    uint8_t stencilContentMask = StencilBits::none;

    for (const auto &[index, passes] : renderQueue) {
        for (const auto &pass : passes) {
            if (pass->getRenderPassConfig().renderTarget != target) {
                continue;
            }
            const auto passConfig = pass->getRenderPassConfig();
            const auto concretePass = std::dynamic_pointer_cast<RenderPass>(pass);
            const auto stencilOptions = concretePass ? concretePass->getStencilOptions() : RenderPassStencilOptions();
            // Render passes can either use a legacy masking object or the explicit stencil lifecycle
            // used by vector-tile masks. Both paths feed the same platform stencil states.
            const auto &maskObject = pass->getMaskingObject();
            const bool hasExplicitMask = maskObject != nullptr && !stencilOptions.isEnabled();
            const bool readsStencil = stencilOptions.read.isEnabled();
            const bool writesStencil = stencilOptions.write.isEnabled();
            const bool hasMask = hasExplicitMask || readsStencil;
            const bool usesStencil = hasMask || passConfig.isPassMasked || writesStencil;
            const uint8_t clearBeforeMask = static_cast<uint8_t>(
                stencilOptions.clearBefore.clearMask |
                (hasExplicitMask || passConfig.isPassMasked ? StencilBits::all : StencilBits::none));
            const uint8_t clearAfterMask = static_cast<uint8_t>(
                stencilOptions.clearAfter.clearMask |
                (hasExplicitMask || passConfig.isPassMasked ? StencilBits::all : StencilBits::none));

            const auto &renderObjects = pass->getRenderObjects();

            if (readsStencil && (stencilContentMask & stencilOptions.read.readMask) != stencilOptions.read.readMask) {
                // A read pass without a preceding write would render against undefined stencil content.
                continue;
            }

            bool prepared = false;

            auto scissoringRect = pass->getScissoringRect();

            for (const auto &renderObject : renderObjects) {
                bool isScreenSpaceCoords = renderObject->isScreenSpaceCoords();

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

                    if (usesStencil && clearBeforeMask != StencilBits::none) {
                        renderingContext->clearStencilMask(clearBeforeMask);
                        stencilContentMask = static_cast<uint8_t>(stencilContentMask & ~clearBeforeMask);
                    }

                    if (hasExplicitMask) {
                        auto maskWriteConfig = RenderPassConfig(passConfig.renderPassIndex, false, passConfig.renderTarget,
                                                                StencilBits::none, StencilBits::none,
                                                                StencilBits::geometryMask, StencilBits::geometryMask);
                        renderingContext->setupStencilWriteMask(maskWriteConfig.stencilWriteMask,
                                                               maskWriteConfig.stencilWriteReference);
                        maskObject->renderAsMask(renderingContext, maskWriteConfig, vpMatrixPointer,
                                                 identityMatrixPointer, origin, factor, isScreenSpaceCoords);
                        stencilContentMask |= StencilBits::geometryMask;
                    }

                    prepared = true;
                }

                const auto &graphicsObject = renderObject->getGraphicsObject();
                const bool objectScreenSpaceCoords = renderObject->isScreenSpaceCoords();
                const auto objectVpMatrixPointer = objectScreenSpaceCoords ? identityMatrixPointer : vpMatrixPointer;
                auto objectMMatrixPointer = identityMatrixPointer;
                auto objectOrigin = objectScreenSpaceCoords ? zeroOrigin : origin;
                std::vector<float> mMatrix;
                if (!objectScreenSpaceCoords && renderObject->hasCustomModelMatrix()) {
                    mMatrix = renderObject->getCustomModelMatrix();
                    objectMMatrixPointer = (int64_t)mMatrix.data();
                }

                if (writesStencil) {
                    // Some layer objects carry an explicit mask interface for the same underlying object
                    // as their graphics interface; others rely on the graphics object also being maskable.
                    auto maskingObject = renderObject->getMaskingObject();
                    if (!maskingObject) {
                        maskingObject = std::dynamic_pointer_cast<MaskingObjectInterface>(graphicsObject);
                    }
                    auto maskWriteConfig = RenderPassConfig(passConfig.renderPassIndex, false, passConfig.renderTarget,
                                                            StencilBits::none, StencilBits::none,
                                                            stencilOptions.write.writeMask,
                                                            stencilOptions.write.reference);
                    if (maskingObject) {
                        renderingContext->setupStencilWriteMask(maskWriteConfig.stencilWriteMask,
                                                               maskWriteConfig.stencilWriteReference);
                        maskingObject->renderAsMask(renderingContext, maskWriteConfig, objectVpMatrixPointer,
                                                    objectMMatrixPointer, objectOrigin, factor, objectScreenSpaceCoords);
                        stencilContentMask |= stencilOptions.write.writeMask;
                    }
                } else {
                    auto readConfig = passConfig;
                    if (readsStencil) {
                        readConfig.stencilReadMask = stencilOptions.read.readMask;
                        readConfig.stencilReadReference = stencilOptions.read.reference;
                    } else if (hasExplicitMask) {
                        readConfig.stencilReadMask = StencilBits::geometryMask;
                        readConfig.stencilReadReference = StencilBits::geometryMask;
                    }
                    graphicsObject->render(renderingContext, readConfig, objectVpMatrixPointer,
                                           objectMMatrixPointer, objectOrigin, hasMask, factor, objectScreenSpaceCoords);
                }
            }

            if (prepared) {
                if (usesStencil && clearAfterMask != StencilBits::none) {
                    renderingContext->clearStencilMask(clearAfterMask);
                    stencilContentMask = static_cast<uint8_t>(stencilContentMask & ~clearAfterMask);
                }

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

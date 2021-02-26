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
    auto mvpMatrixPointer = (int64_t) mvpMatrix.data();

    renderingContext->setupDrawFrame();

    while (!renderQueue.empty()) {
        auto pass = renderQueue.front();

        const auto &graphicsObjects = pass->getGraphicsObjects();
        const auto &customTransforms = pass->getCustomObjectTransforms();
        int numGraphicsObjects = graphicsObjects.size();
        for (int i = 0; i < numGraphicsObjects; i++) {
            const auto &object = graphicsObjects.at(i);
            if (customTransforms.count(i) > 0) {
                object->render(renderingContext, pass->getRenderPassConfig(),
                               (int64_t) customTransforms.at(i).data());
            } else {
                object->render(renderingContext, pass->getRenderPassConfig(), mvpMatrixPointer);
            }
        }

        renderQueue.pop();
    }
}

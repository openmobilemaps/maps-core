/*
 * Copyright (c) 2021 Ubique Innovation AG <https://www.ubique.ch>
 *
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this
 *  file, You can obtain one at https://mozilla.org/MPL/2.0/.
 *
 *  SPDX-License-Identifier: MPL-2.0
 */

#include "Scene.h"
#include "ColorShaderInterface.h"
#include "RenderPass.h"
#include "Renderer.h"

Scene::Scene(const std::shared_ptr<::GraphicsObjectFactoryInterface> &graphicsFactory,
             const std::shared_ptr<::ShaderFactoryInterface> &shaderFactory,
             const std::shared_ptr<::RenderingContextInterface> &renderingContext)
    : graphicsFactory(graphicsFactory)
    , shaderFactory(shaderFactory)
    , renderingContext(renderingContext)
    , renderer(std::make_shared<Renderer>()) {}

void Scene::setCallbackHandler(const std::shared_ptr<SceneCallbackInterface> &callbackInterface) {
    callbackHandler = callbackInterface;
}

void Scene::setCamera(const std::shared_ptr<CameraInterface> &camera) { this->camera = camera; }

std::shared_ptr<::GraphicsObjectFactoryInterface> Scene::getGraphicsFactory() { return graphicsFactory; }

std::shared_ptr<::ShaderFactoryInterface> Scene::getShaderFactory() { return shaderFactory; }

std::shared_ptr<CameraInterface> Scene::getCamera() { return camera; }

std::shared_ptr<RendererInterface> Scene::getRenderer() { return renderer; }

std::shared_ptr<RenderingContextInterface> Scene::getRenderingContext() { return renderingContext; }

void Scene::prepare() {
}

void Scene::drawFrame() {
    if (camera) {
        renderer->drawFrame(renderingContext, camera);
    }
}

void Scene::compute() {
    if (camera) {
        renderer->compute(renderingContext, camera);
    }
}

void Scene::clear() {}

void Scene::invalidate() {
    if (auto handler = callbackHandler) {
        handler->invalidate();
    }
}

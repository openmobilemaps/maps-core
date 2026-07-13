/*
 * Copyright (c) 2021 Ubique Innovation AG <https://www.ubique.ch>
 *
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this
 *  file, You can obtain one at https://mozilla.org/MPL/2.0/.
 *
 *  SPDX-License-Identifier: MPL-2.0
 */

#include "TexturedPolygonLayer.h"
#include "GraphicsObjectFactoryInterface.h"
#include "GraphicsObjectInterface.h"
#include "LambdaTask.h"
#include "MapInterface.h"
#include "RenderObject.h"
#include "RenderPass.h"
#include "RenderPassInterface.h"
#include "RenderingContextInterface.h"
#include "SchedulerInterface.h"
#include "ShaderFactoryInterface.h"
#include "ShaderProgramInterface.h"

TexturedPolygonLayer::TexturedPolygonLayer() = default;

void TexturedPolygonLayer::setPolygon(const ::PolygonCoord &polygon, const ::RectCoord &textureBounds) {
    {
        std::lock_guard<std::recursive_mutex> lock(stateMutex);
        pendingPolygon = polygon;
        pendingTextureBounds = textureBounds;
        polygonApplied = false;
    }
    if (mapInterface) {
        applyPolygon();
        rebuildRenderPasses();
        mapInterface->invalidate();
    }
}

void TexturedPolygonLayer::loadTexture(const std::shared_ptr<::TextureHolderInterface> &texture) {
    {
        std::lock_guard<std::recursive_mutex> lock(stateMutex);
        pendingTexture = texture;
        textureApplied = false;
    }
    if (mapInterface) {
        applyTexture();
        rebuildRenderPasses();
        mapInterface->invalidate();
    }
}

void TexturedPolygonLayer::setAlpha(float newAlpha) {
    alpha = newAlpha;
    auto currentShader = this->shader;
    if (currentShader) {
        currentShader->updateAlpha(newAlpha);
    }
    if (mapInterface) {
        mapInterface->invalidate();
    }
}

float TexturedPolygonLayer::getAlpha() {
    return alpha;
}

void TexturedPolygonLayer::setRenderPassIndex(int32_t index) {
    renderPassIndex = index;
    if (mapInterface) {
        rebuildRenderPasses();
        mapInterface->invalidate();
    }
}

std::shared_ptr<::LayerInterface> TexturedPolygonLayer::asLayerInterface() {
    return shared_from_this();
}

void TexturedPolygonLayer::update() {}

std::vector<std::shared_ptr<::RenderPassInterface>> TexturedPolygonLayer::buildRenderPasses() {
    if (isHidden) {
        return {};
    }
    std::lock_guard<std::recursive_mutex> lock(renderPassMutex);
    return renderPasses;
}

void TexturedPolygonLayer::onAdded(const std::shared_ptr<MapInterface> &mapInterface, int32_t /*layerIndex*/) {
    this->mapInterface = mapInterface;
    setupGraphics();
}

void TexturedPolygonLayer::onRemoved() {
    auto graphicsObject = layerObject ? layerObject->getGraphicsObject() : nullptr;
    if (graphicsObject && graphicsObject->isReady()) {
        graphicsObject->clear();
    }
    {
        std::lock_guard<std::recursive_mutex> lock(renderPassMutex);
        renderPasses.clear();
    }
    layerObject = nullptr;
    texturedPolygon = nullptr;
    shader = nullptr;
    mapInterface = nullptr;
}

void TexturedPolygonLayer::pause() {
    auto graphicsObject = layerObject ? layerObject->getGraphicsObject() : nullptr;
    if (graphicsObject && graphicsObject->isReady()) {
        graphicsObject->clear();
    }
}

void TexturedPolygonLayer::resume() {
    {
        std::lock_guard<std::recursive_mutex> lock(stateMutex);
        textureApplied = false;
    }
    applyTexture();
    rebuildRenderPasses();
    if (mapInterface) {
        mapInterface->invalidate();
    }
}

void TexturedPolygonLayer::hide() {
    isHidden = true;
    if (mapInterface) {
        mapInterface->invalidate();
    }
}

void TexturedPolygonLayer::show() {
    isHidden = false;
    if (mapInterface) {
        mapInterface->invalidate();
    }
}

void TexturedPolygonLayer::setupGraphics() {
    auto mapInterface = this->mapInterface;
    auto shaderFactory = mapInterface ? mapInterface->getShaderFactory() : nullptr;
    auto objectFactory = mapInterface ? mapInterface->getGraphicsObjectFactory() : nullptr;
    if (!shaderFactory || !objectFactory) {
        return;
    }

    bool is3d = mapInterface->is3d();

    shader = shaderFactory->createAlphaShader();
    shader->updateAlpha(alpha);

    texturedPolygon = objectFactory->createTexturedPolygon(shader->asShaderProgramInterface());
    layerObject = std::make_shared<Textured2dLayerObject>(texturedPolygon, shader, mapInterface, is3d);

    applyPolygon();
    applyTexture();
    rebuildRenderPasses();
}

void TexturedPolygonLayer::applyPolygon() {
    std::optional<::PolygonCoord> polygon;
    std::optional<::RectCoord> textureBounds;
    {
        std::lock_guard<std::recursive_mutex> lock(stateMutex);
        if (polygonApplied || !pendingPolygon || !pendingTextureBounds) {
            return;
        }
        polygon = pendingPolygon;
        textureBounds = pendingTextureBounds;
    }

    auto layerObject = this->layerObject;
    if (!layerObject) {
        return;
    }

    layerObject->setPolygons({*polygon}, *textureBounds);

    {
        std::lock_guard<std::recursive_mutex> lock(stateMutex);
        polygonApplied = true;
    }
}

void TexturedPolygonLayer::applyTexture() {
    std::shared_ptr<::TextureHolderInterface> texture;
    {
        std::lock_guard<std::recursive_mutex> lock(stateMutex);
        if (textureApplied || !pendingTexture) {
            return;
        }
        texture = pendingTexture;
    }

    auto mapInterface = this->mapInterface;
    auto scheduler = mapInterface ? mapInterface->getScheduler() : nullptr;
    auto layerObject = this->layerObject;
    if (!scheduler || !layerObject) {
        return;
    }

    std::weak_ptr<TexturedPolygonLayer> weakSelf =
        std::dynamic_pointer_cast<TexturedPolygonLayer>(shared_from_this());
    scheduler->addTask(std::make_shared<LambdaTask>(
        TaskConfig("TexturedPolygonLayer_loadTexture", 0, TaskPriority::NORMAL, ExecutionEnvironment::GRAPHICS),
        [weakSelf, texture] {
            auto self = weakSelf.lock();
            if (!self) {
                return;
            }
            auto mapInterface = self->mapInterface;
            auto renderingContext = mapInterface ? mapInterface->getRenderingContext() : nullptr;
            auto layerObject = self->layerObject;
            if (!renderingContext || !layerObject) {
                return;
            }
            layerObject->getGraphicsObject()->setup(renderingContext);
            layerObject->loadTexture(renderingContext, texture);
            {
                std::lock_guard<std::recursive_mutex> lock(self->stateMutex);
                self->textureApplied = true;
            }
            mapInterface->invalidate();
        }));
}

void TexturedPolygonLayer::rebuildRenderPasses() {
    auto layerObject = this->layerObject;
    if (!layerObject) {
        std::lock_guard<std::recursive_mutex> lock(renderPassMutex);
        renderPasses.clear();
        return;
    }

    std::vector<std::shared_ptr<::RenderObjectInterface>> renderObjects = {
        std::make_shared<RenderObject>(layerObject->getGraphicsObject())};
    auto newRenderPass = std::make_shared<RenderPass>(
        RenderPassConfig(renderPassIndex, false, renderTarget, StencilBits::none, StencilBits::none, StencilBits::none, StencilBits::none), renderObjects);

    std::lock_guard<std::recursive_mutex> lock(renderPassMutex);
    renderPasses = {newRenderPass};
}

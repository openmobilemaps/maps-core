/*
 * Copyright (c) 2021 Ubique Innovation AG <https://www.ubique.ch>
 *
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this
 *  file, You can obtain one at https://mozilla.org/MPL/2.0/.
 *
 *  SPDX-License-Identifier: MPL-2.0
 */

#include "MapScene.h"
#include "CoordinateConversionHelper.h"
#include "DateHelper.h"
#include "DefaultTouchHandlerInterface.h"
#include "LambdaTask.h"
#include "LayerInterface.h"
#include "MapCallbackInterface.h"
#include "MapCamera2dInterface.h"
#include "MapReadyCallbackInterface.h"
#include "TouchInterface.h"
#include "Logger.h"
#include <algorithm>

#include "Tiled2dMapRasterLayer.h"

MapScene::MapScene(std::shared_ptr<SceneInterface> scene, const MapConfig &mapConfig,
                   const std::shared_ptr<::SchedulerInterface> &scheduler, float pixelDensity)
    : scene(scene)
    , mapConfig(mapConfig)
    , scheduler(scheduler)
    , conversionHelper(std::make_shared<CoordinateConversionHelper>(mapConfig.mapCoordinateSystem)) {
    // add default touch handler
    setTouchHandler(DefaultTouchHandlerInterface::create(scheduler, pixelDensity));

    // workaround to use sharedpointer to self from constructor
    auto ptr = std::shared_ptr<MapScene>(this, [](MapScene *) {});

    // add default camera
    setCamera(MapCamera2dInterface::create(ptr, pixelDensity));
}

MapScene::~MapScene() {
    std::lock_guard<std::recursive_mutex> lock(layersMutex);
    for (const auto &layerEntry : layers) {
        layerEntry.second->onRemoved();
    }
}

std::shared_ptr<::GraphicsObjectFactoryInterface> MapScene::getGraphicsObjectFactory() { return scene->getGraphicsFactory(); }

std::shared_ptr<::ShaderFactoryInterface> MapScene::getShaderFactory() { return scene->getShaderFactory(); }

std::shared_ptr<::SchedulerInterface> MapScene::getScheduler() { return scheduler; }

std::shared_ptr<::RenderingContextInterface> MapScene::getRenderingContext() { return scene->getRenderingContext(); }

MapConfig MapScene::getMapConfig() { return mapConfig; }

std::shared_ptr<::CoordinateConversionHelperInterface> MapScene::getCoordinateConverterHelper() { return conversionHelper; }

void MapScene::setCallbackHandler(const std::shared_ptr<MapCallbackInterface> &callbackInterface) {
    scene->setCallbackHandler(shared_from_this());
    callbackHandler = callbackInterface;
}

void MapScene::setCamera(const std::shared_ptr<::MapCamera2dInterface> &camera) {
    if (touchHandler && std::dynamic_pointer_cast<TouchInterface>(camera)) {
        auto prevCamera = std::dynamic_pointer_cast<TouchInterface>(scene->getCamera());
        if (prevCamera) {
            touchHandler->removeListener(prevCamera);
        }
        auto newCamera = std::dynamic_pointer_cast<TouchInterface>(camera);
        touchHandler->addListener(newCamera);
    }
    this->camera = camera;
    scene->setCamera(camera->asCameraInterface());
}

std::shared_ptr<::MapCamera2dInterface> MapScene::getCamera() { return camera; }

void MapScene::setTouchHandler(const std::shared_ptr<::TouchHandlerInterface> &touchHandler) {
    auto currentCamera = std::dynamic_pointer_cast<TouchInterface>(scene->getCamera());
    if (this->touchHandler && currentCamera) {
        this->touchHandler->removeListener(currentCamera);
    }
    this->touchHandler = touchHandler;
    if (currentCamera) {
        this->touchHandler->addListener(currentCamera);
    }
}

std::shared_ptr<::TouchHandlerInterface> MapScene::getTouchHandler() { return touchHandler; }

std::vector<std::shared_ptr<LayerInterface>> MapScene::getLayers() {
    std::vector<std::shared_ptr<LayerInterface>> layersList;
    for (const auto &l : layers) {
        layersList.emplace_back(l.second);
    }
    return layersList;
};

void MapScene::addLayer(const std::shared_ptr<::LayerInterface> &layer) {
    removeLayer(layer);
    layer->onAdded(shared_from_this());
    std::lock_guard<std::recursive_mutex> lock(layersMutex);
    int topIndex = -1;
    if (!layers.empty())
        topIndex = layers.rbegin()->first;
    layers[topIndex + 1] = layer;
}

void MapScene::insertLayerAt(const std::shared_ptr<LayerInterface> &layer, int32_t atIndex) {
    {
        std::lock_guard<std::recursive_mutex> lock(layersMutex);
        if (layers.count(atIndex) > 0 && layers.at(atIndex) == layer) {
            return;
        }
    }
    removeLayer(layer);
    layer->onAdded(shared_from_this());
    std::lock_guard<std::recursive_mutex> lock(layersMutex);
    if (layers.count(atIndex) > 0) {
        layers[atIndex]->onRemoved();
    }
    layers[atIndex] = layer;
};

void MapScene::insertLayerAbove(const std::shared_ptr<LayerInterface> &layer, const std::shared_ptr<LayerInterface> &above) {
    removeLayer(layer);
    layer->onAdded(shared_from_this());
    std::lock_guard<std::recursive_mutex> lock(layersMutex);
    int targetIndex = -1;
    for (const auto &[i, l] : layers) {
        if (l == above) {
            targetIndex = i;
            break;
        }
    }
    if (targetIndex < 0) {
        throw std::invalid_argument("MapScene does not contain above layer");
    }
    std::map<int, std::shared_ptr<LayerInterface>> newLayers;
    for (auto iter = layers.rbegin(); iter != layers.rend(); iter++) {
        newLayers[iter->first > targetIndex ? iter->first + 1 : iter->first] = iter->second;
    }
    newLayers[targetIndex + 1] = layer;
    layers = newLayers;
};

void MapScene::insertLayerBelow(const std::shared_ptr<LayerInterface> &layer, const std::shared_ptr<LayerInterface> &below) {
    removeLayer(layer);
    layer->onAdded(shared_from_this());
    std::lock_guard<std::recursive_mutex> lock(layersMutex);
    int targetIndex = -1;
    for (const auto &[i, l] : layers) {
        if (l == below) {
            targetIndex = i;
            break;
        }
    }
    if (targetIndex < 0) {
        throw std::invalid_argument("MapScene does not contain below layer");
    }
    std::map<int, std::shared_ptr<LayerInterface>> newLayers;
    for (auto iter = layers.rbegin(); iter != layers.rend(); iter++) {
        newLayers[iter->first >= targetIndex ? iter->first + 1 : iter->first] = iter->second;
    }
    newLayers[targetIndex] = layer;
    layers = newLayers;
};

void MapScene::removeLayer(const std::shared_ptr<::LayerInterface> &layer) {
    {
        std::lock_guard<std::recursive_mutex> lock(layersMutex);
        int targetIndex = -1;
        for (const auto &[i, l] : layers) {
            if (l == layer) {
                targetIndex = i;
                break;
            }
        }
        if (targetIndex >= 0) {
            layers.erase(targetIndex);
            layer->onRemoved();
        }
    }
}

void MapScene::setViewportSize(const ::Vec2I &size) {
    getRenderingContext()->setViewportSize(size);
    camera->asCameraInterface()->viewportSizeChanged();
}

void MapScene::setBackgroundColor(const Color &color) { getRenderingContext()->setBackgroundColor(color); }

void MapScene::invalidate() {
    if (isInvalidated.test_and_set())
        return;

    if (auto handler = callbackHandler) {
        handler->invalidate();
    }
}

void MapScene::drawFrame() {
    isInvalidated.clear();

    if (!isResumed)
        return;

    auto const camera = this->camera;
    if (camera) {
        camera->update();
    }

    std::vector<std::shared_ptr<::RenderTargetTexture>> additionalTargets;

    {
        std::lock_guard<std::recursive_mutex> lock(layersMutex);

        for (const auto &layer : layers) {
            layer.second->update();
            for (const auto &target : layer.second->additionalTargets()) {
                additionalTargets.push_back(target);
            }
        }

        for (const auto &layer : layers) {
            for (const auto &renderPass : layer.second->buildRenderPasses()) {
                scene->getRenderer()->addToRenderQueue(renderPass);
            }
        }
    }

    scene->drawFrame(additionalTargets);
}

void MapScene::resume() {
    isResumed = true;
    std::weak_ptr<MapScene> weakSelfPtr = weak_from_this();
    scheduler->addTask(
        std::make_shared<LambdaTask>(TaskConfig("MapScene_resume", 0, TaskPriority::NORMAL, ExecutionEnvironment::GRAPHICS), [weakSelfPtr] {
            auto selfPtr = weakSelfPtr.lock();
            if (!selfPtr) { return; }

            std::lock_guard<std::recursive_mutex> lock(selfPtr->layersMutex);
            for (const auto &layer : selfPtr->layers) {
                layer.second->resume();
            }
        }));
}

void MapScene::pause() {
    isResumed = false;

    std::weak_ptr<MapScene> weakSelfPtr = weak_from_this();
    scheduler->addTask(
        std::make_shared<LambdaTask>(TaskConfig("MapScene_pause", 0, TaskPriority::NORMAL, ExecutionEnvironment::GRAPHICS), [weakSelfPtr] {
            auto selfPtr = weakSelfPtr.lock();
            if (!selfPtr) { return; }

            std::lock_guard<std::recursive_mutex> lock(selfPtr->layersMutex);
            for (const auto &layer : selfPtr->layers) {
                layer.second->pause();
            }
        }));
}

void MapScene::drawReadyFrame(const ::RectCoord &bounds, float timeout,
                              const std::shared_ptr<MapReadyCallbackInterface> &callbacks) {

    // for now we only support drawing a ready frame, therefore
    // we disable animations in the layers
    for (const auto &layer : layers) {
        layer.second->enableAnimations(false);
    }

    auto state = LayerReadyState::NOT_READY;

    invalidate();
    callbacks->stateDidUpdate(state);

    auto camera = getCamera();
    camera->moveToBoundingBox(bounds, 0.0, false, std::nullopt);
    camera->freeze(true);

    invalidate();
    callbacks->stateDidUpdate(state);

    long long timeoutTimestamp = DateHelper::currentTimeMillis() + (long long)(timeout * 1000);

    while (state == LayerReadyState::NOT_READY) {
        state = getLayersReadyState();

        auto now = DateHelper::currentTimeMillis();
        if (now > timeoutTimestamp) {
            state = LayerReadyState::TIMEOUT_ERROR;
        }

        invalidate();
        callbacks->stateDidUpdate(state);
    }
    // re-enable animations if the map scene is used not only for
    // drawReadyFrame
    camera->freeze(false);
    for (const auto &layer : layers) {
        layer.second->enableAnimations(true);
    }
}

LayerReadyState MapScene::getLayersReadyState() {
    std::lock_guard<std::recursive_mutex> lock(layersMutex);

    for (const auto &layer : layers) {
        auto state = layer.second->isReadyToRenderOffscreen();
        if (state == LayerReadyState::READY) {
            continue;
        }

        return state;
    }

    return LayerReadyState::READY;
}

void MapScene::forceReload() {
    std::lock_guard<std::recursive_mutex> lock(layersMutex);

    for (const auto &[index, layer] : layers) {
        layer->forceReload();
    }
}

std::shared_ptr<RenderTargetTexture> MapScene::createRenderTargetTexture() {
    auto context = getRenderingContext();
    return context->createRenderTargetTexture();
}

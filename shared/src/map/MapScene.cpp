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
#include "IndexedLayer.h"
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

    scheduler->setSchedulerGraphicsTaskCallbacks(ptr);

    // add default camera
    setCamera(MapCamera2dInterface::create(ptr, pixelDensity));
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

std::vector<std::shared_ptr<IndexedLayerInterface>> MapScene::getLayersIndexed() {
    std::vector<std::shared_ptr<IndexedLayerInterface>> layersList;
    for (const auto &l : layers) {
        layersList.emplace_back(std::make_shared<IndexedLayer>(l.first, l.second));
    }
    return layersList;
}

void MapScene::addLayer(const std::shared_ptr<::LayerInterface> &layer) {
    removeLayer(layer);
    {
        auto weakSelfPtr = weak_from_this();
        scheduler->addTask(
                std::make_shared<LambdaTask>(
                        TaskConfig("MapScene_addLayer", 0, TaskPriority::NORMAL, ExecutionEnvironment::GRAPHICS),
                        [weakSelfPtr, layer] {
                            auto strongSelf = weakSelfPtr.lock();
                            if (!strongSelf) {
                                return;
                            }
                            int atIndex;
                            {
                                std::lock_guard<std::recursive_mutex> lock(strongSelf->layersMutex);
                                int topIndex = -1;
                                if (!strongSelf->layers.empty())
                                    topIndex = strongSelf->layers.rbegin()->first;
                                atIndex = topIndex + 1;
                            }

                            layer->onAdded(strongSelf, atIndex);

                            {
                                std::lock_guard<std::recursive_mutex> lock(strongSelf->layersMutex);
                                strongSelf->layers[atIndex] = layer;
                            }

                            if (strongSelf->isResumed) {
                                layer->resume();
                            }

                            strongSelf->invalidate();
                        }));
    }
}

void MapScene::insertLayerAt(const std::shared_ptr<LayerInterface> &layer, int32_t atIndex) {
    {
        std::lock_guard<std::recursive_mutex> lock(layersMutex);
        if (layers.count(atIndex) > 0 && layers.at(atIndex) == layer) {
            return;
        }
    }
    removeLayer(layer);
    {
        auto weakSelfPtr = weak_from_this();
        scheduler->addTask(
                std::make_shared<LambdaTask>(
                        TaskConfig("MapScene_insertLayerAt", 0, TaskPriority::NORMAL, ExecutionEnvironment::GRAPHICS),
                        [weakSelfPtr, layer, atIndex] {
                            auto strongSelf = weakSelfPtr.lock();
                            if (!strongSelf) {
                                return;
                            }

                            std::shared_ptr<LayerInterface> oldLayer;
                            {
                                std::lock_guard<std::recursive_mutex> lock(strongSelf->layersMutex);
                                oldLayer = strongSelf->layers.count(atIndex) > 0 ? strongSelf->layers[atIndex] : nullptr;
                            }

                            if (oldLayer == layer) {
                                return;
                            }

                            bool isResumed = strongSelf->isResumed;
                            if (oldLayer) {
                                if (isResumed) {
                                    oldLayer->pause();
                                }
                                oldLayer->onRemoved();
                            }

                            layer->onAdded(strongSelf, atIndex);

                            {
                                std::lock_guard<std::recursive_mutex> lock(strongSelf->layersMutex);
                                strongSelf->layers[atIndex] = layer;
                            }

                            if (isResumed) {
                                layer->resume();
                            }

                            strongSelf->invalidate();
                        }));
    }
};

void MapScene::insertLayerAbove(const std::shared_ptr<LayerInterface> &layer, const std::shared_ptr<LayerInterface> &above) {
    removeLayer(layer);

    auto weakSelfPtr = weak_from_this();
    scheduler->addTask(
            std::make_shared<LambdaTask>(
                    TaskConfig("MapScene_insertLayerAbove", 0, TaskPriority::NORMAL, ExecutionEnvironment::GRAPHICS),
                    [weakSelfPtr, layer, above] {
                        auto strongSelf = weakSelfPtr.lock();
                        if (!strongSelf) {
                            return;
                        }

                        std::map<int, std::shared_ptr<LayerInterface>> newLayers;

                        int atIndex;
                        {
                            std::lock_guard<std::recursive_mutex> lock(strongSelf->layersMutex);
                            int targetIndex = -1;
                            for (const auto &[i, l]: strongSelf->layers) {
                                if (l == above) {
                                    targetIndex = i;
                                    break;
                                }
                            }
                            if (targetIndex < 0) {
                                throw std::invalid_argument("MapScene does not contain above layer");
                            }
                            for (auto iter = strongSelf->layers.rbegin(); iter != strongSelf->layers.rend(); iter++) {
                                newLayers[iter->first > targetIndex ? iter->first + 1 : iter->first] = iter->second;
                            }
                            atIndex = targetIndex + 1;
                            newLayers[atIndex] = layer;
                        }
                        layer->onAdded(strongSelf, atIndex);

                        {
                            std::lock_guard<std::recursive_mutex> lock(strongSelf->layersMutex);
                            strongSelf->layers = newLayers;
                        }

                        if (strongSelf->isResumed) {
                            layer->resume();
                        }

                        strongSelf->invalidate();
                    }));
};

void MapScene::insertLayerBelow(const std::shared_ptr<LayerInterface> &layer, const std::shared_ptr<LayerInterface> &below) {
    removeLayer(layer);

    std::weak_ptr<LayerInterface> newLayer = layer;
    auto weakSelfPtr = weak_from_this();
    scheduler->addTask(
            std::make_shared<LambdaTask>(
                    TaskConfig("MapScene_insertLayerBelow", 0, TaskPriority::NORMAL, ExecutionEnvironment::GRAPHICS),
                    [weakSelfPtr, layer, below] {
                        auto strongSelf = weakSelfPtr.lock();
                        if (!strongSelf) {
                            return;
                        }

                        std::map<int, std::shared_ptr<LayerInterface>> newLayers;
                        int atIndex;
                        {
                            std::lock_guard<std::recursive_mutex> lock(strongSelf->layersMutex);
                            int targetIndex = -1;
                            for (const auto &[i, l]: strongSelf->layers) {
                                if (l == below) {
                                    targetIndex = i;
                                    break;
                                }
                            }
                            if (targetIndex < 0) {
                                throw std::invalid_argument("MapScene does not contain below layer");
                            }

                            for (auto iter = strongSelf->layers.rbegin(); iter != strongSelf->layers.rend(); iter++) {
                                newLayers[iter->first >= targetIndex ? iter->first + 1 : iter->first] = iter->second;
                            }
                            newLayers[targetIndex] = layer;
                        }

                        layer->onAdded(strongSelf, atIndex);

                        {
                            std::lock_guard<std::recursive_mutex> lock(strongSelf->layersMutex);
                            strongSelf->layers = newLayers;
                        }


                        if (strongSelf->isResumed) {
                            layer->resume();
                        }

                        strongSelf->invalidate();
                    }));
};

void MapScene::removeLayer(const std::shared_ptr<::LayerInterface> &layer) {
    {
        auto scheduler = this->scheduler;
        auto weakSelf = weak_from_this();
        if (scheduler) {
            scheduler->addTask(
                    std::make_shared<LambdaTask>(
                            TaskConfig("MapScene_removeLayer", 0, TaskPriority::NORMAL, ExecutionEnvironment::GRAPHICS),
                            [weakSelf, layer] {
                                auto strongSelf = weakSelf.lock();
                                if (!strongSelf) {
                                    return;
                                }

                                {
                                    std::lock_guard<std::recursive_mutex> lock(strongSelf->layersMutex);
                                    int targetIndex = -1;
                                    for (const auto &[i, l]: strongSelf->layers) {
                                        if (l == layer) {
                                            targetIndex = i;
                                            break;
                                        }
                                    }
                                    if (targetIndex < 0) {
                                        return;
                                    }

                                    strongSelf->layers.erase(targetIndex);
                                }

                                if (strongSelf->isResumed) {
                                    layer->pause();
                                }
                                layer->onRemoved();

                                strongSelf->invalidate();
                            }));
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

void MapScene::prepare() {
    isInvalidated.clear();

    if (scheduler && scheduler->hasSeparateGraphicsInvocation()) {
        if (scheduler->runGraphicsTasks()) {
            invalidate();
        }
    }

    if (!isResumed)
        return;

    auto const camera = this->camera;
    if (camera) {
        camera->update();
    }

    {
        std::lock_guard<std::recursive_mutex> lock(layersMutex);
        for (const auto &layer : layers) {
            layer.second->update();
        }

        for (const auto &layer : layers) {
            for (const auto &renderPass : layer.second->buildRenderPasses()) {
                scene->getRenderer()->addToRenderQueue(renderPass);
            }

            for (const auto &computePass : layer.second->buildComputePasses()) {
                scene->getRenderer()->addToComputeQueue(computePass);
            }
        }
    }
}

void MapScene::compute() {
    scene->compute();
}

void MapScene::drawFrame() {
    scene->drawFrame();
}

void MapScene::resume() {
    if (isResumed) {
        return;
    }

    scheduler->resume();

    std::lock_guard<std::recursive_mutex> lock(layersMutex);
    for (const auto &layer : layers) {
        layer.second->resume();
    }

    isResumed = true;
    callbackHandler->onMapResumed();
}

void MapScene::pause() {
    if (!isResumed) {
        return;
    }
    isResumed = false;

    std::lock_guard<std::recursive_mutex> lock(layersMutex);
    for (const auto &layer : layers) {
        layer.second->pause();
    }

    scheduler->pause();
}

void MapScene::destroy() {
    std::lock_guard<std::recursive_mutex> lock(layersMutex);
    for (const auto &layer : layers) {
        if (isResumed) {
            layer.second->pause();
        }
        layer.second->onRemoved();
    }
    layers.clear();

    scheduler->destroy();
    scheduler = nullptr;
    callbackHandler = nullptr;
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
    camera->moveToBoundingBox(bounds, 0.0, false, std::nullopt, std::nullopt);
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

void MapScene::requestGraphicsTaskExecution() {
    invalidate();
}

/*
 * Copyright (c) 2021 Ubique Innovation AG <https://www.ubique.ch>
 *
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this
 *  file, You can obtain one at https://mozilla.org/MPL/2.0/.
 *
 *  SPDX-License-Identifier: MPL-2.0
 */

#include "Tiled2dMapRasterLayer.h"
#include "LambdaTask.h"
#include "MapConfig.h"
#include "RenderConfigInterface.h"
#include "RenderPass.h"
#include "RenderObject.h"
#include "MapCamera2dInterface.h"
#include <map>
#include <Logger.h>

Tiled2dMapRasterLayer::Tiled2dMapRasterLayer(const std::shared_ptr<::Tiled2dMapLayerConfig> &layerConfig,
                                             const std::shared_ptr<::LoaderInterface> & tileLoader)
: Tiled2dMapLayer(layerConfig), textureLoader(tileLoader), alpha(1.0) {}

Tiled2dMapRasterLayer::Tiled2dMapRasterLayer(const std::shared_ptr<::Tiled2dMapLayerConfig> &layerConfig,
                                             const std::shared_ptr<::LoaderInterface> & tileLoader,
                                             const std::shared_ptr<::MaskingObjectInterface> & mask): Tiled2dMapLayer(layerConfig), textureLoader(tileLoader), alpha(1.0),
                                                 mask(mask) {}

void Tiled2dMapRasterLayer::onAdded(const std::shared_ptr<::MapInterface> &mapInterface) {
    rasterSource = std::make_shared<Tiled2dMapRasterSource>(mapInterface->getMapConfig(), layerConfig,
                                                            mapInterface->getCoordinateConverterHelper(),
                                                            mapInterface->getScheduler(), textureLoader, shared_from_this());
    setSourceInterface(rasterSource);
    Tiled2dMapLayer::onAdded(mapInterface);

    mapInterface->getTouchHandler()->addListener(shared_from_this());
}

void Tiled2dMapRasterLayer::onRemoved() {
    auto mapInterface = this->mapInterface;
    if (mapInterface) {
        mapInterface->getTouchHandler()->removeListener(shared_from_this());
    }
    pause();
    Tiled2dMapLayer::onRemoved();
}

std::shared_ptr<::LayerInterface> Tiled2dMapRasterLayer::asLayerInterface() { return shared_from_this(); }

void Tiled2dMapRasterLayer::update() {
    auto mapInterface = this->mapInterface;
    if (mapInterface && mask) {
        if (!mask->asGraphicsObject()->isReady()) mask->asGraphicsObject()->setup(mapInterface->getRenderingContext());
    }
    std::lock_guard<std::recursive_mutex> overlayLock(updateMutex);
    for (auto const &tile : tileObjectMap) {
        if (tile.second)
            tile.second->update();
    }
}

std::vector<std::shared_ptr<::RenderPassInterface>> Tiled2dMapRasterLayer::buildRenderPasses() {
    std::lock_guard<std::recursive_mutex> overlayLock(renderPassMutex);
    return renderPasses;
}

void Tiled2dMapRasterLayer::pause() {
    Tiled2dMapLayer::pause();
    if (mask) {
        if (mask->asGraphicsObject()->isReady()) mask->asGraphicsObject()->clear();
    }
    std::lock_guard<std::recursive_mutex> overlayLock(updateMutex);
    for (const auto &tileObject : tileObjectMap) {
        if (tileObject.second && tileObject.second->getQuadObject()->asGraphicsObject()->isReady()) tileObject.second->getQuadObject()->asGraphicsObject()->clear();
    }
}

void Tiled2dMapRasterLayer::resume() {
    Tiled2dMapLayer::resume();
    auto mapInterface = this->mapInterface;
    auto renderingContext = mapInterface ? mapInterface->getRenderingContext() : nullptr;
    if (!renderingContext) {
        return;
    }

    if (mask) {
        if (!mask->asGraphicsObject()->isReady()) mask->asGraphicsObject()->setup(renderingContext);
    }
    std::lock_guard<std::recursive_mutex> overlayLock(updateMutex);
    for (const auto &tileObject : tileObjectMap) {
        if (tileObject.second) {
            auto rectangle = tileObject.second->getQuadObject();
            rectangle->asGraphicsObject()->setup(renderingContext);
            rectangle->loadTexture(renderingContext, tileObject.first.textureHolder);
        }
    }
}

void Tiled2dMapRasterLayer::onTilesUpdated() {
    auto lockSelfPtr = std::static_pointer_cast<Tiled2dMapRasterLayer>(shared_from_this());
    auto mapInterface = lockSelfPtr ? lockSelfPtr->mapInterface : nullptr;
    auto graphicsFactory = mapInterface ? mapInterface->getGraphicsObjectFactory() : nullptr;
    auto shaderFactory = mapInterface ? mapInterface->getShaderFactory() : nullptr;
    if (!graphicsFactory || !shaderFactory) {
        return;
    }

    {
        auto currentTileInfos = rasterSource->getCurrentTiles();
        std::vector<const std::pair<const Tiled2dMapRasterTileInfo, std::shared_ptr<Textured2dLayerObject>>> tilesToSetup;
        std::vector<std::shared_ptr<Textured2dLayerObject>> tilesToClean;
        std::map<int, std::vector<std::shared_ptr<RenderObjectInterface>>> renderPassObjectMap;

        {
            std::lock_guard<std::recursive_mutex> overlayLock(updateMutex);

            std::unordered_set<Tiled2dMapRasterTileInfo> tilesToAdd;
            for (const auto &rasterTileInfo : currentTileInfos) {
                if (tileObjectMap.count(rasterTileInfo) == 0) {
                    tilesToAdd.insert(rasterTileInfo);
                }
            }

            std::unordered_set<Tiled2dMapRasterTileInfo> tilesToRemove;
            for (const auto &tileEntry : tileObjectMap) {
                if (currentTileInfos.count(tileEntry.first) == 0)
                    tilesToRemove.insert(tileEntry.first);
            }

            if (tilesToAdd.empty() && tilesToRemove.empty()) return;


            for (const auto &tile : tilesToAdd) {
                auto alphaShader = shaderFactory->createAlphaShader();
                auto tileObject = std::make_shared<Textured2dLayerObject>(
                        graphicsFactory->createQuad(alphaShader->asShaderProgramInterface()), alphaShader, mapInterface);
                if (layerConfig->getZoomInfo().numDrawPreviousLayers == 0 || !animationsEnabled) {
                    tileObject->setAlpha(alpha);
                } else {
                    tileObject->beginAlphaAnimation(0.0, alpha, 150);
                }
                tileObject->setRectCoord(tile.tileInfo.bounds);
                tilesToSetup.emplace_back(std::make_pair(tile, tileObject));
                tileObjectMap[tile] = tileObject;
            }

            for (const auto &tile : tilesToRemove) {
                auto tileObject = tileObjectMap.at(tile);
                tilesToClean.emplace_back(tileObject);
                tileObjectMap.erase(tile);
            }

            std::vector<std::pair<int, std::shared_ptr<Textured2dLayerObject>>> mapEntries;
            for (auto &entry : tileObjectMap) {
                mapEntries.emplace_back(std::make_pair(entry.first.tileInfo.zoomLevel, entry.second));
            }
            sort(mapEntries.begin(), mapEntries.end(),
                 [=](std::pair<int, std::shared_ptr<Textured2dLayerObject>> &a,
                     std::pair<int, std::shared_ptr<Textured2dLayerObject>> &b) { return a.first < b.first; });

            for (const auto &objectEntry : mapEntries) {
                objectEntry.second->getQuadObject()->asGraphicsObject();
                for (const auto &config : objectEntry.second->getRenderConfig()) {
                    renderPassObjectMap[config->getRenderIndex()].push_back(
                            std::make_shared<RenderObject>(config->getGraphicsObject()));
                }
            }
        }

        std::vector<std::shared_ptr<RenderPassInterface>> newRenderPasses;
        for (const auto &passEntry : renderPassObjectMap) {
            std::shared_ptr<RenderPass> renderPass =
                    std::make_shared<RenderPass>(RenderPassConfig(passEntry.first), passEntry.second, mask);
            renderPass->setScissoringRect(scissorRect);
            newRenderPasses.push_back(renderPass);
        }
        {
            std::lock_guard<std::recursive_mutex> overlayLock(renderPassMutex);
            renderPasses = newRenderPasses;
        }

        std::weak_ptr<Tiled2dMapRasterLayer> weakSelfPtr = std::dynamic_pointer_cast<Tiled2dMapRasterLayer>(shared_from_this());
        mapInterface->getScheduler()->addTask(std::make_shared<LambdaTask>(
                TaskConfig("Tiled2dMapRasterLayer_onTilesUpdated", 0, TaskPriority::NORMAL, ExecutionEnvironment::GRAPHICS),
                [weakSelfPtr, tilesToSetup, tilesToClean] {
                    auto selfPtr = weakSelfPtr.lock();
                    if (selfPtr) selfPtr->setupTiles(tilesToSetup, tilesToClean);
                }));
    }
}

void Tiled2dMapRasterLayer::setupTiles(
        const std::vector<const std::pair<const Tiled2dMapRasterTileInfo, std::shared_ptr<Textured2dLayerObject>>> &tilesToSetup,
        const std::vector<std::shared_ptr<Textured2dLayerObject>> &tilesToClean) {
    auto mapInterface = this->mapInterface;
    auto renderingContext = mapInterface ? mapInterface->getRenderingContext() : nullptr;
    if (!renderingContext) return;

    {
        std::lock_guard<std::recursive_mutex> overlayLock(updateMutex);
        for (const auto &tile : tilesToSetup) {
            const auto &tileInfo = tile.first;
            const auto &tileObject = tile.second;
            if (!tileObject || !tileObjectMap.count(tile.first)) continue;
            tileObject->getQuadObject()->asGraphicsObject()->setup(renderingContext);

            if (tileInfo.textureHolder) {
                tileObject->getQuadObject()->loadTexture(renderingContext, tileInfo.textureHolder);
            }
        }
    }

    for (const auto &tileObject : tilesToClean) {
        if (!tileObject) continue;
        tileObject->getQuadObject()->removeTexture();
    }

    mapInterface->invalidate();
}

void Tiled2dMapRasterLayer::generateRenderPasses() {
    std::map<int, std::vector<std::shared_ptr<RenderObjectInterface>>> renderPassObjectMap;
    std::vector<std::pair<int, std::shared_ptr<Textured2dLayerObject>>> mapEntries;
    {
        std::lock_guard<std::recursive_mutex> overlayLock(updateMutex);
        for (auto &entry : tileObjectMap) {
            mapEntries.emplace_back(std::make_pair(entry.first.tileInfo.zoomLevel, entry.second));
        }
    }
    sort(mapEntries.begin(), mapEntries.end(),
         [=](std::pair<int, std::shared_ptr<Textured2dLayerObject>> &a,
             std::pair<int, std::shared_ptr<Textured2dLayerObject>> &b) { return a.first < b.first; });

    for (const auto &objectEntry : mapEntries) {
        for (auto config : objectEntry.second->getRenderConfig()) {
            renderPassObjectMap[config->getRenderIndex()].push_back(
                    std::make_shared<RenderObject>(config->getGraphicsObject()));
        }
    }

    std::vector<std::shared_ptr<RenderPassInterface>> newRenderPasses;
    for (const auto &passEntry : renderPassObjectMap) {
        std::shared_ptr<RenderPass> renderPass =
                std::make_shared<RenderPass>(RenderPassConfig(passEntry.first), passEntry.second, mask);
        renderPass->setScissoringRect(scissorRect);
        newRenderPasses.push_back(renderPass);
    }
    {
        std::lock_guard<std::recursive_mutex> overlayLock(renderPassMutex);
        renderPasses = newRenderPasses;
    }
}

void Tiled2dMapRasterLayer::setCallbackHandler(const std::shared_ptr<Tiled2dMapRasterLayerCallbackInterface> &handler) {
    callbackHandler = handler;
}

void Tiled2dMapRasterLayer::removeCallbackHandler() {
    callbackHandler = nullptr;
}

std::shared_ptr<Tiled2dMapRasterLayerCallbackInterface> Tiled2dMapRasterLayer::getCallbackHandler() {
    return callbackHandler;
}

void Tiled2dMapRasterLayer::setAlpha(double alpha) {
    this->alpha = alpha;
    {
        std::lock_guard<std::recursive_mutex> overlayLock(updateMutex);
        for (const auto &tileObject : tileObjectMap) {
            tileObject.second->setAlpha(alpha);
        }
    }

    if (mapInterface) mapInterface->invalidate();
}

double Tiled2dMapRasterLayer::getAlpha() {
    return alpha;
}

bool Tiled2dMapRasterLayer::onClickConfirmed(const Vec2F &posScreen) {
    auto callbackHandler = this->callbackHandler;
    return (callbackHandler) && callbackHandler->onClickConfirmed(mapInterface->getCamera()->coordFromScreenPosition(posScreen));
}

bool Tiled2dMapRasterLayer::onLongPress(const Vec2F &posScreen) {
    auto callbackHandler = this->callbackHandler;
    return (callbackHandler) && callbackHandler->onLongPress(mapInterface->getCamera()->coordFromScreenPosition(posScreen));
}

void Tiled2dMapRasterLayer::setScissorRect(const std::optional<::RectI> & scissorRect) {
    this->scissorRect = scissorRect;
    generateRenderPasses();
    auto mapInterface = this->mapInterface;
    if (mapInterface) {
        mapInterface->invalidate();
    }
}

void Tiled2dMapRasterLayer::setMaskingObject(const std::shared_ptr<::MaskingObjectInterface> &maskingObject) {
    mask = maskingObject;
    generateRenderPasses();
    auto mapInterface = this->mapInterface;
    if (mapInterface) {
        mapInterface->invalidate();
    }
}

void Tiled2dMapRasterLayer::setMinZoomLevelIdentifier(std::optional<int32_t> value) {
    Tiled2dMapLayer::setMinZoomLevelIdentifier(value);
}

std::optional<int32_t> Tiled2dMapRasterLayer::getMinZoomLevelIdentifier() {
    return Tiled2dMapLayer::getMinZoomLevelIdentifier();
}

void Tiled2dMapRasterLayer::setMaxZoomLevelIdentifier(std::optional<int32_t> value) {
    Tiled2dMapLayer::setMaxZoomLevelIdentifier(value);
}

std::optional<int32_t> Tiled2dMapRasterLayer::getMaxZoomLevelIdentifier() {
    return Tiled2dMapLayer::getMaxZoomLevelIdentifier();
}

void Tiled2dMapRasterLayer::enableAnimations(bool enabled) {
    animationsEnabled = enabled;
}

LayerReadyState Tiled2dMapRasterLayer::isReadyToRenderOffscreen() {
    std::lock_guard<std::recursive_mutex> overlayLock(updateMutex);

    auto sourceReady = Tiled2dMapLayer::isReadyToRenderOffscreen();
    if(sourceReady != LayerReadyState::READY) {
        return sourceReady;
    }

    for(auto& to : tileObjectMap) {
        if(!to.second->getQuadObject()->asGraphicsObject()->isReady()) {
            return LayerReadyState::NOT_READY;
        }
    }

    return LayerReadyState::READY;
}

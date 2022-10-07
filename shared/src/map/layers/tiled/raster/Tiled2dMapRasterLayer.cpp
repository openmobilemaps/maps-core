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
#include "MapCamera2dInterface.h"
#include "MapConfig.h"
#include "RenderConfigInterface.h"
#include "RenderObject.h"
#include "RenderPass.h"
#include "PolygonCompare.h"
#include <Logger.h>
#include <map>

Tiled2dMapRasterLayer::Tiled2dMapRasterLayer(const std::shared_ptr<::Tiled2dMapLayerConfig> &layerConfig,
                                             const std::vector<std::shared_ptr<::LoaderInterface>> & tileLoaders)
        : Tiled2dMapLayer(), layerConfig(layerConfig), tileLoaders(tileLoaders), alpha(1.0) {}

Tiled2dMapRasterLayer::Tiled2dMapRasterLayer(const std::shared_ptr<::Tiled2dMapLayerConfig> &layerConfig,
                                             const std::vector<std::shared_ptr<::LoaderInterface>> & tileLoaders,
                                             const std::shared_ptr<::MaskingObjectInterface> &mask)
        : Tiled2dMapLayer(), layerConfig(layerConfig), tileLoaders(tileLoaders), alpha(1.0), mask(mask) {}

Tiled2dMapRasterLayer::Tiled2dMapRasterLayer(const std::shared_ptr<::Tiled2dMapLayerConfig> &layerConfig,
                                             const std::vector<std::shared_ptr<::LoaderInterface>> & tileLoaders,
                                             const std::shared_ptr<::ShaderProgramInterface> &shader)
        : Tiled2dMapLayer(), layerConfig(layerConfig), tileLoaders(tileLoaders), alpha(1.0), shader(shader) {}

void Tiled2dMapRasterLayer::onAdded(const std::shared_ptr<::MapInterface> &mapInterface) {
    rasterSource = std::make_shared<Tiled2dMapRasterSource>(
            mapInterface->getMapConfig(), layerConfig, mapInterface->getCoordinateConverterHelper(), mapInterface->getScheduler(),
                                                            tileLoaders, shared_from_this(), mapInterface->getCamera()->getScreenDensityPpi());
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
        if (!mask->asGraphicsObject()->isReady())
            mask->asGraphicsObject()->setup(mapInterface->getRenderingContext());
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
        if (mask->asGraphicsObject()->isReady())
            mask->asGraphicsObject()->clear();
    }
    std::lock_guard<std::recursive_mutex> overlayLock(updateMutex);
    for (const auto &tileObject : tileObjectMap) {
        if (tileObject.second && tileObject.second->getQuadObject()->asGraphicsObject()->isReady())
            tileObject.second->getQuadObject()->asGraphicsObject()->clear();
    }
    for (const auto &tileMask : tileMaskMap) {
        if (tileMask.second.maskObject && tileMask.second.maskObject->getPolygonObject()->asGraphicsObject()->isReady())
            tileMask.second.maskObject->getPolygonObject()->asGraphicsObject()->clear();
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
        if (!mask->asGraphicsObject()->isReady())
            mask->asGraphicsObject()->setup(renderingContext);
    }
    std::lock_guard<std::recursive_mutex> overlayLock(updateMutex);
    for (const auto &tileObject : tileObjectMap) {
        if (tileObject.second) {
            auto rectangle = tileObject.second->getQuadObject();
            rectangle->asGraphicsObject()->setup(renderingContext);
            rectangle->loadTexture(renderingContext, tileObject.first.textureHolder);
        }
    }
    for (const auto &tileMask : tileMaskMap) {
        if (tileMask.second.maskObject) {
            auto polygon = tileMask.second.maskObject->getPolygonObject();
            polygon->asGraphicsObject()->setup(renderingContext);
        }
    }
}

void Tiled2dMapRasterLayer::setT(int32_t t) {
    Tiled2dMapLayer::setT(t);
    onTilesUpdated();
}

bool Tiled2dMapRasterLayer::shouldLoadTile(const Tiled2dMapTileInfo& tileInfo){
    return abs(tileInfo.t - curT) < 10;
}

void Tiled2dMapRasterLayer::onTilesUpdated() {
    auto lockSelfPtr = std::static_pointer_cast<Tiled2dMapRasterLayer>(shared_from_this());
    auto mapInterface = lockSelfPtr ? lockSelfPtr->mapInterface : nullptr;
    auto graphicsFactory = mapInterface ? mapInterface->getGraphicsObjectFactory() : nullptr;
    auto coordinateConverterHelper = mapInterface ? mapInterface->getCoordinateConverterHelper() : nullptr;
    auto shaderFactory = mapInterface ? mapInterface->getShaderFactory() : nullptr;
    if (!graphicsFactory || !shaderFactory) {
        return;
    }

    {
        if (updateFlag.test_and_set()) {
            return;
        }

        auto currentTileInfos = rasterSource->getCurrentTiles();
        std::vector<const std::pair<const Tiled2dMapRasterTileInfo, std::shared_ptr<Textured2dLayerObject>>> tilesToSetup, tilesToClean;
        std::vector<const std::shared_ptr<MaskingObjectInterface>> newMaskObjects;
        std::vector<const std::shared_ptr<MaskingObjectInterface>> obsoleteMaskObjects;

        {
            std::lock_guard<std::recursive_mutex> overlayLock(updateMutex);
            updateFlag.clear();

            std::unordered_set<Tiled2dMapRasterTileInfo> tilesToAdd;
            for (const auto &rasterTileInfo : currentTileInfos) {
                if (shouldLoadTile(rasterTileInfo.tileInfo)) {
                    auto it = tileObjectMap.find(rasterTileInfo);
                    if (it == tileObjectMap.end()) {
                        tilesToAdd.insert(rasterTileInfo);
                    }
                }
            }

            std::unordered_map<Tiled2dMapTileInfo, Tiled2dMapLayerMaskWrapper> newTileMasks;

            std::unordered_set<Tiled2dMapRasterTileInfo> tilesToRemove;
            for (const auto &tileEntry : tileObjectMap) {
                if (currentTileInfos.count(tileEntry.first) == 0 || !shouldLoadTile(tileEntry.first.tileInfo)){
                    tilesToRemove.insert(tileEntry.first);
                }
            }

            if (layerConfig->getZoomInfo().maskTile) {
                for (const auto &tileEntry : tileObjectMap) {
                    if (tilesToRemove.count(tileEntry.first) == 0) {
                        const auto &curTile = currentTileInfos.find(tileEntry.first);
                        const size_t hash = std::hash<std::vector<::PolygonCoord>>()(curTile->masks);

                        if (tileMaskMap[tileEntry.first.tileInfo].polygonHash != hash) {
                            const auto &tileMask = std::make_shared<PolygonMaskObject>(graphicsFactory,
                                                                                       coordinateConverterHelper);

                            tileMask->setPolygons(curTile->masks);
                            newTileMasks[tileEntry.first.tileInfo] = Tiled2dMapLayerMaskWrapper(tileMask, hash);
                        }
                    }
                }
            }

            if (tilesToAdd.empty() && tilesToRemove.empty() && newTileMasks.empty()) return;

            auto const &zoomInfo = layerConfig->getZoomInfo();
            for (const auto &tile : tilesToAdd) {
                std::shared_ptr<Textured2dLayerObject> tileObject;
                if (shader) {
                    tileObject = std::make_shared<Textured2dLayerObject>(graphicsFactory->createQuad(shader), nullptr, mapInterface);
                } else {
                    auto alphaShader = shaderFactory->createAlphaShader();
                    tileObject = std::make_shared<Textured2dLayerObject>(
                            graphicsFactory->createQuad(alphaShader->asShaderProgramInterface()), alphaShader, mapInterface);
                }
                if (zoomInfo.numDrawPreviousLayers == 0 || !animationsEnabled || zoomInfo.maskTile) {
                    tileObject->setAlpha(alpha);
                } else {
                    tileObject->beginAlphaAnimation(0.0, alpha, 150);
                }
                tileObject->setRectCoord(tile.tileInfo.bounds);
                tilesToSetup.emplace_back(std::make_pair(tile, tileObject));

                tileObjectMap[tile] = tileObject;

                if (newTileMasks.count(tile.tileInfo) == 0 && layerConfig->getZoomInfo().maskTile) {
                    const auto &tileMask = std::make_shared<PolygonMaskObject>(graphicsFactory,
                                                                               coordinateConverterHelper);
                    const size_t hash = std::hash<std::vector<::PolygonCoord>>()(tile.masks);
                    tileMask->setPolygons(tile.masks);
                    newTileMasks[tile.tileInfo] = Tiled2dMapLayerMaskWrapper(tileMask, hash);
                }
            }

            for (const auto &newMaskEntry : newTileMasks) {
                if (tileMaskMap.count(newMaskEntry.first) > 0) {
                    obsoleteMaskObjects.emplace_back(tileMaskMap.at(newMaskEntry.first).maskObject->getPolygonObject()->asMaskingObject());
                }
                tileMaskMap[newMaskEntry.first] = newMaskEntry.second;
                newMaskObjects.emplace_back(newMaskEntry.second.maskObject->getPolygonObject()->asMaskingObject());
            }

            for (const auto &tile : tilesToRemove) {
                auto tileObject = tileObjectMap.at(tile);
                tilesToClean.emplace_back(std::make_pair(tile, tileObject));
            }
        }

        std::weak_ptr<Tiled2dMapRasterLayer> weakSelfPtr = std::dynamic_pointer_cast<Tiled2dMapRasterLayer>(shared_from_this());
        mapInterface->getScheduler()->addTask(std::make_shared<LambdaTask>(
                TaskConfig("Tiled2dMapRasterLayer_onTilesUpdated", 0, TaskPriority::NORMAL, ExecutionEnvironment::GRAPHICS),
                [weakSelfPtr, tilesToSetup, tilesToClean, newMaskObjects, obsoleteMaskObjects] {
                    auto selfPtr = weakSelfPtr.lock();
                    if (selfPtr) {
                        selfPtr->updateMaskObjects(newMaskObjects, obsoleteMaskObjects);
                        selfPtr->setupTiles(tilesToSetup, tilesToClean);
                    }
                }));
    }
}


void Tiled2dMapRasterLayer::updateMaskObjects(const std::vector<const std::shared_ptr<MaskingObjectInterface>> &newMaskObjects,
                                              const std::vector<const std::shared_ptr<MaskingObjectInterface>> &obsoleteMaskObjects) {
    if (!mapInterface) return;
    std::lock_guard<std::recursive_mutex> overlayLock(updateMutex);
    for (const auto &mask : newMaskObjects) {
        const auto &object = mask->asGraphicsObject();
        if (!object->isReady()) object->setup(mapInterface->getRenderingContext());
    }
    for (const auto &mask : obsoleteMaskObjects) {
        const auto &object = mask->asGraphicsObject();
        if (object->isReady()) object->clear();
    }
}


void Tiled2dMapRasterLayer::setupTiles(
        const std::vector<const std::pair<const Tiled2dMapRasterTileInfo, std::shared_ptr<Textured2dLayerObject>>> &tilesToSetup,
        const std::vector<const std::pair<const Tiled2dMapRasterTileInfo, std::shared_ptr<Textured2dLayerObject>>> &tilesToClean) {
    auto mapInterface = this->mapInterface;
    auto renderingContext = mapInterface ? mapInterface->getRenderingContext() : nullptr;
    if (!renderingContext)
        return;

    std::vector<const Tiled2dMapTileInfo> tilesReady;
    {
        std::lock_guard<std::recursive_mutex> overlayLock(updateMutex);
        for (const auto &tile : tilesToSetup) {
            const auto &tileInfo = tile.first;
            const auto &tileObject = tile.second;
            if (!tileObject) {
                continue;
            }

            tileObject->getQuadObject()->asGraphicsObject()->setup(renderingContext);

            if (tileInfo.textureHolder) {
                tileObject->getQuadObject()->loadTexture(renderingContext, tileInfo.textureHolder);
            }
            // the texture holder can be empty, some tileserver serve 0 byte textures
            tilesReady.push_back(tileInfo.tileInfo);
        }

        for (const auto &[tile, tileObject] : tilesToClean) {
            tileObjectMap.erase(tile);
        }
    }

    generateRenderPasses();

    // remove the texture after setting the new renderpasses
    // otherwise it could happen, that in the meantime a already existing object gets rendered
    {
        std::lock_guard<std::recursive_mutex> overlayLock(updateMutex);
        for (const auto &[tile, tileObject] : tilesToClean) {
            if (!tileObject) continue;
            tileObject->getQuadObject()->removeTexture();
        }
    }

    rasterSource->setTilesReady(tilesReady);

    mapInterface->invalidate();

}

void Tiled2dMapRasterLayer::generateRenderPasses() {

    auto mapInterface = this->mapInterface;
    auto renderingContext = mapInterface ? mapInterface->getRenderingContext() : nullptr;
    if (!renderingContext)
        return;


    std::vector<std::shared_ptr<RenderPassInterface>> newRenderPasses;

    {
        std::lock_guard<std::recursive_mutex> overlayLock(updateMutex);

        for (const auto &entry : tileObjectMap) {
            if (entry.first.tileInfo.t != curT) {
                continue;
            }
            auto const &quadObject = entry.second->getQuadObject();
            auto const renderObject = std::make_shared<RenderObject>(quadObject->asGraphicsObject());


            if (layerConfig->getZoomInfo().maskTile) {
                const auto &mask = tileMaskMap.at(entry.first.tileInfo).maskObject;
                mask->getPolygonObject()->asGraphicsObject()->setup(renderingContext);

                std::shared_ptr<RenderPass> renderPass =
                std::make_shared<RenderPass>(RenderPassConfig(0),
                                             std::vector<std::shared_ptr<::RenderObjectInterface>>{renderObject},
                                             mask->getPolygonObject()->asMaskingObject());
                renderPass->setScissoringRect(scissorRect);
                newRenderPasses.push_back(renderPass);
            }else{
                std::shared_ptr<RenderPass> renderPass = std::make_shared<RenderPass>(RenderPassConfig(0),
                                                                                      std::vector<std::shared_ptr<::RenderObjectInterface>>{
                    renderObject});
                renderPass->setScissoringRect(scissorRect);

                newRenderPasses.push_back(renderPass);
            }

            //TODO: general mask would no longer work now, we would have to merge the tile-mask with the layer-mask
            if (mask) {
                std::shared_ptr<RenderPass> renderPass =
                std::make_shared<RenderPass>(RenderPassConfig(0),
                                             std::vector<std::shared_ptr<::RenderObjectInterface>>{renderObject}, mask);
                renderPass->setScissoringRect(scissorRect);
                newRenderPasses.push_back(renderPass);
            }
        }

    }

    {
        std::lock_guard<std::recursive_mutex> overlayLock(renderPassMutex);
        renderPasses = newRenderPasses;
    }

}

void Tiled2dMapRasterLayer::setCallbackHandler(const std::shared_ptr<Tiled2dMapRasterLayerCallbackInterface> &handler) {
    callbackHandler = handler;
}

void Tiled2dMapRasterLayer::removeCallbackHandler() { callbackHandler = nullptr; }

std::shared_ptr<Tiled2dMapRasterLayerCallbackInterface> Tiled2dMapRasterLayer::getCallbackHandler() { return callbackHandler; }

void Tiled2dMapRasterLayer::setAlpha(double alpha) {
    if (this->alpha == alpha) {
        return;
    }
    this->alpha = alpha;
    {
        std::lock_guard<std::recursive_mutex> overlayLock(updateMutex);
        for (const auto &tileObject : tileObjectMap) {
            tileObject.second->setAlpha(alpha);
        }
    }

    if (mapInterface)
        mapInterface->invalidate();
}

double Tiled2dMapRasterLayer::getAlpha() { return alpha; }

bool Tiled2dMapRasterLayer::onClickConfirmed(const Vec2F &posScreen) {
    auto callbackHandler = this->callbackHandler;
    return (callbackHandler) && callbackHandler->onClickConfirmed(mapInterface->getCamera()->coordFromScreenPosition(posScreen));
}

bool Tiled2dMapRasterLayer::onLongPress(const Vec2F &posScreen) {
    auto callbackHandler = this->callbackHandler;
    return (callbackHandler) && callbackHandler->onLongPress(mapInterface->getCamera()->coordFromScreenPosition(posScreen));
}

void Tiled2dMapRasterLayer::setScissorRect(const std::optional<::RectI> &scissorRect) {
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

std::optional<int32_t> Tiled2dMapRasterLayer::getMinZoomLevelIdentifier() { return Tiled2dMapLayer::getMinZoomLevelIdentifier(); }

void Tiled2dMapRasterLayer::setMaxZoomLevelIdentifier(std::optional<int32_t> value) {
    Tiled2dMapLayer::setMaxZoomLevelIdentifier(value);
}

std::optional<int32_t> Tiled2dMapRasterLayer::getMaxZoomLevelIdentifier() { return Tiled2dMapLayer::getMaxZoomLevelIdentifier(); }

void Tiled2dMapRasterLayer::enableAnimations(bool enabled) { animationsEnabled = enabled; }

LayerReadyState Tiled2dMapRasterLayer::isReadyToRenderOffscreen() {
    std::lock_guard<std::recursive_mutex> overlayLock(updateMutex);

    auto sourceReady = Tiled2dMapLayer::isReadyToRenderOffscreen();
    if (sourceReady != LayerReadyState::READY) {
        return sourceReady;
    }

    for (auto &to : tileObjectMap) {
        if (!to.second->getQuadObject()->asGraphicsObject()->isReady()) {
            return LayerReadyState::NOT_READY;
        }
    }

    return LayerReadyState::READY;
}

std::shared_ptr<::Tiled2dMapLayerConfig> Tiled2dMapRasterLayer::getConfig() {
    return layerConfig;
}

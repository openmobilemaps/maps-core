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
#include <map>

Tiled2dMapRasterLayer::Tiled2dMapRasterLayer(const std::shared_ptr<::Tiled2dMapLayerConfig> &layerConfig,
                                             const std::shared_ptr<::TextureLoaderInterface> &textureLoader)
    : Tiled2dMapLayer(layerConfig)
    , textureLoader(textureLoader) {}

void Tiled2dMapRasterLayer::onAdded(const std::shared_ptr<::MapInterface> &mapInterface) {
    alphaShader = mapInterface->getShaderFactory()->createAlphaShader();

    rasterSource = std::make_shared<Tiled2dMapRasterSource>(mapInterface->getMapConfig(), layerConfig,
                                                            mapInterface->getCoordinateConverterHelper(),
                                                            mapInterface->getScheduler(), textureLoader, shared_from_this());
    setSourceInterface(rasterSource);
    Tiled2dMapLayer::onAdded(mapInterface);
}

void Tiled2dMapRasterLayer::onRemoved() {
    Tiled2dMapLayer::onRemoved();
    pause();
}

std::shared_ptr<::LayerInterface> Tiled2dMapRasterLayer::asLayerInterface() { return shared_from_this(); }

void Tiled2dMapRasterLayer::update() {}

std::vector<std::shared_ptr<::RenderPassInterface>> Tiled2dMapRasterLayer::buildRenderPasses() { return renderPasses; }

void Tiled2dMapRasterLayer::pause() {
    rasterSource->pause();
    std::lock_guard<std::recursive_mutex> overlayLock(updateMutex);
    for (const auto &tileObject : tileObjectMap) {
        tileObject.second->getQuadObject()->asGraphicsObject()->clear();
    }
}

void Tiled2dMapRasterLayer::resume() {
    rasterSource->resume();
    auto renderingContext = mapInterface->getRenderingContext();
    std::lock_guard<std::recursive_mutex> overlayLock(updateMutex);
    for (const auto &tileObject : tileObjectMap) {
        auto rectangle = tileObject.second->getQuadObject();
        rectangle->asGraphicsObject()->setup(renderingContext);
        rectangle->loadTexture(tileObject.first.textureHolder);
    }
}

void Tiled2dMapRasterLayer::onTilesUpdated() {
    mapInterface->getScheduler()->addTask(std::make_shared<LambdaTask>(
        TaskConfig("Tiled2dMapRasterLayer_onTilesUpdated", 0, TaskPriority::NORMAL, ExecutionEnvironment::GRAPHICS), [=] {
            std::lock_guard<std::recursive_mutex> overlayLock(updateMutex);

            auto currentTileInfos = rasterSource->getCurrentTiles();

            std::unordered_set<Tiled2dMapRasterTileInfo> tilesToAdd;
            for (const auto &rasterTileInfo : currentTileInfos) {
                if (!tileObjectMap[rasterTileInfo]) {
                    tilesToAdd.insert(rasterTileInfo);
                }
            }

            std::unordered_set<Tiled2dMapRasterTileInfo> tilesToRemove;
            for (const auto &tileEntry : tileObjectMap) {
                if (currentTileInfos.count(tileEntry.first) == 0)
                    tilesToRemove.insert(tileEntry.first);
            }

            auto renderingContext = mapInterface->getRenderingContext();
            auto graphicsFactory = mapInterface->getGraphicsObjectFactory();

            for (const auto &tile : tilesToAdd) {

                auto tileObject = std::make_shared<Textured2dLayerObject>(
                    graphicsFactory->createQuad(alphaShader->asShaderProgramInterface()), alphaShader,
                    mapInterface->getCoordinateConverterHelper());
                tileObject->setAlpha(1.0);

                tileObject->setRectCoord(tile.tileInfo.bounds);
                tileObject->getQuadObject()->asGraphicsObject()->setup(renderingContext);

                if (tile.textureHolder) {
                    tileObject->getQuadObject()->loadTexture(tile.textureHolder);
                }

                tileObjectMap[tile] = tileObject;
            }

            for (const auto &tile : tilesToRemove) {
                auto tileObject = tileObjectMap[tile];
                tileObject->getQuadObject()->removeTexture();
                tileObjectMap.erase(tile);
            }

            std::map<int, std::vector<std::shared_ptr<GraphicsObjectInterface>>> renderPassObjectMap;
            std::vector<std::pair<int, std::shared_ptr<Textured2dLayerObject>>> mapEntries;
            for (auto &entry : tileObjectMap) {
                mapEntries.push_back(std::make_pair(entry.first.tileInfo.zoomLevel, entry.second));
            }
            sort(mapEntries.begin(), mapEntries.end(),
                 [=](std::pair<int, std::shared_ptr<Textured2dLayerObject>> &a,
                     std::pair<int, std::shared_ptr<Textured2dLayerObject>> &b) { return a.first < b.first; });

            for (const auto &objectEntry : mapEntries) {
                objectEntry.second->getQuadObject()->asGraphicsObject();
                for (auto config : objectEntry.second->getRenderConfig()) {
                    renderPassObjectMap[config->getRenderIndex()].push_back(config->getGraphicsObject());
                }
            }

            std::vector<std::shared_ptr<RenderPassInterface>> newRenderPasses;
            for (const auto &passEntry : renderPassObjectMap) {
                std::shared_ptr<RenderPass> renderPass =
                    std::make_shared<RenderPass>(RenderPassConfig(passEntry.first), passEntry.second);
                newRenderPasses.push_back(renderPass);
            }
            renderPasses = newRenderPasses;

            mapInterface->invalidate();
        }));
}

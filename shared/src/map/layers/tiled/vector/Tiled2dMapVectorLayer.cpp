/*
 * Copyright (c) 2021 Ubique Innovation AG <https://www.ubique.ch>
 *
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this
 *  file, You can obtain one at https://mozilla.org/MPL/2.0/.
 *
 *  SPDX-License-Identifier: MPL-2.0
 */

#include "vtzero/vector_tile.hpp"
#include "Tiled2dMapVectorLayer.h"
#include "Tiled2dMapVectorLayerConfig.h"
#include "Tiled2dMapVectorRasterSubLayerConfig.h"
#include "Tiled2dMapRasterLayerInterface.h"
#include "RasterVectorLayerDescription.h"
#include "LineVectorLayerDescription.h"
#include "PolygonVectorLayerDescription.h"
#include "SymbolVectorLayerDescription.h"
#include "Tiled2dMapVectorLineSubLayer.h"
#include "Tiled2dMapVectorPolygonSubLayer.h"
#include "VectorTileGeometryHandler.h"
#include "Tiled2dMapVectorSymbolSubLayer.h"
#include "Polygon2dInterface.h"
#include "MapCamera2dInterface.h"
#include "QuadMaskObject.h"
#include "PolygonMaskObject.h"
#include "CoordinatesUtil.h"
#include "RenderPass.h"

Tiled2dMapVectorLayer::Tiled2dMapVectorLayer(const std::shared_ptr<VectorMapDescription> &mapDescription,
                                             const std::shared_ptr<::LoaderInterface> &tileLoader,
                                             const std::shared_ptr<::FontLoaderInterface> &fontLoader) :
        Tiled2dMapLayer(std::make_shared<Tiled2dMapVectorLayerConfig>(mapDescription)),
        vectorTileLoader(tileLoader),
        fontLoader(fontLoader),
        sublayers() {
    for (auto const &layerDesc: mapDescription->layers) {
        switch (layerDesc->getType()) {
            case VectorLayerType::raster: {
                auto rasterDesc = std::static_pointer_cast<RasterVectorLayerDescription>(layerDesc);
                auto layerConfig = std::make_shared<Tiled2dMapVectorRasterSubLayerConfig>(rasterDesc);
                auto layer = Tiled2dMapRasterLayerInterface::create(layerConfig, tileLoader);
                sublayers.push_back(layer->asLayerInterface());
                break;
            }
            case VectorLayerType::line: {
                auto lineDesc = std::static_pointer_cast<LineVectorLayerDescription>(layerDesc);
                auto layer = std::make_shared<Tiled2dMapVectorLineSubLayer>(lineDesc);
                sublayers.push_back(layer);
                const auto sourceId = lineDesc->sourceId;
                if (sourceLayerMap.count(sourceId) == 0) {
                    sourceLayerMap[sourceId] = {layer};
                } else {
                    sourceLayerMap[lineDesc->sourceId].push_back(layer);
                }
                break;
            }
            case VectorLayerType::polygon: {
                auto polyDesc = std::static_pointer_cast<PolygonVectorLayerDescription>(layerDesc);
                auto layer = std::make_shared<Tiled2dMapVectorPolygonSubLayer>(polyDesc);
                sublayers.push_back(layer);
                const auto sourceId = polyDesc->sourceId;
                if (sourceLayerMap.count(sourceId) == 0) {
                    sourceLayerMap[sourceId] = {layer};
                } else {
                    sourceLayerMap[polyDesc->sourceId].push_back(layer);
                }
                break;
            }
            case VectorLayerType::symbol: {
                auto symbolDesc = std::static_pointer_cast<SymbolVectorLayerDescription>(layerDesc);
                auto layer = std::make_shared<Tiled2dMapVectorSymbolSubLayer>(fontLoader, symbolDesc);
                sublayers.push_back(layer);
                const auto sourceId = symbolDesc->sourceId;
                if (sourceLayerMap.count(sourceId) == 0) {
                    sourceLayerMap[sourceId] = {layer};
                } else {
                    sourceLayerMap[sourceId].push_back(layer);
                }
                break;
            }
        }
    }
}

std::shared_ptr<::LayerInterface> Tiled2dMapVectorLayer::asLayerInterface() {
    return shared_from_this();
}

void Tiled2dMapVectorLayer::update() {
    for (auto const &layer: sublayers) {
        layer->update();
    }
}

std::vector<std::shared_ptr<::RenderPassInterface>> Tiled2dMapVectorLayer::buildRenderPasses() {
    std::lock_guard<std::recursive_mutex> tilesReadyLock(tilesReadyMutex);
    std::vector<std::shared_ptr<RenderPassInterface>> newPasses;
    for (const auto &layer: sublayers) {
        std::vector<std::shared_ptr<RenderPassInterface>> sublayerPasses;
        if (std::dynamic_pointer_cast<Tiled2dMapVectorSubLayer>(layer) != nullptr) {
            sublayerPasses = std::dynamic_pointer_cast<Tiled2dMapVectorSubLayer>(layer)->buildRenderPasses(tilesReady);
        } else {
            sublayerPasses = layer->buildRenderPasses();
        }
        newPasses.insert(newPasses.end(), sublayerPasses.begin(), sublayerPasses.end());
    }

    return newPasses;
}

void Tiled2dMapVectorLayer::onAdded(const std::shared_ptr<::MapInterface> &mapInterface) {

    std::unordered_set<std::string> layersToDecode;

    for (auto const& [sourceLayer, val] : sourceLayerMap)
    {
        layersToDecode.insert(sourceLayer);
    }

    vectorTileSource = std::make_shared<Tiled2dMapVectorSource>(mapInterface->getMapConfig(), layerConfig,
                                                                mapInterface->getCoordinateConverterHelper(),
                                                                mapInterface->getScheduler(), vectorTileLoader,
                                                                shared_from_this(),
                                                                layersToDecode,
                                                                mapInterface->getCamera()->getScreenDensityPpi());

    setSourceInterface(vectorTileSource);
    Tiled2dMapLayer::onAdded(mapInterface);
    mapInterface->getTouchHandler()->addListener(shared_from_this());

    for (auto const &layer: sublayers) {
        layer->onAdded(mapInterface);
        auto vectorSubLayer = std::dynamic_pointer_cast<Tiled2dMapVectorSubLayer>(layer);
        if (vectorSubLayer) {
            vectorSubLayer->setTilesReadyDelegate(std::dynamic_pointer_cast<Tiled2dMapVectorLayerReadyInterface>(shared_from_this()));
        }
    }

    // TODO: make sure current tiles ready is up to date
}

void Tiled2dMapVectorLayer::onRemoved() {
    auto const mapInterface = this->mapInterface;
    if (mapInterface) {
        mapInterface->getTouchHandler()->removeListener(shared_from_this());
    }
    Tiled2dMapLayer::onRemoved();
    pause();

    for (auto const &layer: sublayers) {
        layer->onRemoved();
    }
}

void Tiled2dMapVectorLayer::pause() {
    vectorTileSource->pause();
    {
        std::lock_guard<std::recursive_mutex> overlayLock(updateMutex);
        for (const auto &tileMask : tileMaskMap) {
            if (tileMask.second && tileMask.second->getPolygonObject()->asGraphicsObject()->isReady()) tileMask.second->getPolygonObject()->asGraphicsObject()->clear();
        }
    }
    for (auto const &layer: sublayers) {
        layer->pause();
    }

    {
        std::lock_guard<std::recursive_mutex> tilesReadyLock(tilesReadyMutex);
        tilesReady.clear();
    }
}

void Tiled2dMapVectorLayer::resume() {
    vectorTileSource->resume();
    {
        std::lock_guard<std::recursive_mutex> overlayLock(updateMutex);
        const auto &context = mapInterface->getRenderingContext();
        for (const auto &tileMask : tileMaskMap) {
            if (tileMask.second && !tileMask.second->getPolygonObject()->asGraphicsObject()->isReady()) tileMask.second->getPolygonObject()->asMaskingObject()->asGraphicsObject()->setup(context);
        }
    }
    for (auto const &layer: sublayers) {
        layer->resume();
    }
}

void Tiled2dMapVectorLayer::onTilesUpdated() {

    auto lockSelfPtr = std::static_pointer_cast<Tiled2dMapVectorLayer>(shared_from_this());
    auto mapInterface = lockSelfPtr ? lockSelfPtr->mapInterface : nullptr;
    {
        std::lock_guard<std::recursive_mutex> overlayLock(updateMutex);

        auto graphicsFactory = mapInterface ? mapInterface->getGraphicsObjectFactory() : nullptr;
        auto coordinateConverterHelper = mapInterface ? mapInterface->getCoordinateConverterHelper() : nullptr;
        auto shaderFactory = mapInterface ? mapInterface->getShaderFactory() : nullptr;
        if (!graphicsFactory || !shaderFactory) {
            return;
        }

        auto currentTileInfos = vectorTileSource->getCurrentTiles();

        std::unordered_set<Tiled2dMapVectorTileInfo> tilesToAdd;
        std::unordered_set<Tiled2dMapVectorTileInfo> tilesToKeep;
        for (const auto &vectorTileInfo : currentTileInfos) {
            if (tileSet.count(vectorTileInfo) == 0) {
                tilesToAdd.insert(vectorTileInfo);
            } else {
                tilesToKeep.insert(vectorTileInfo);
            }
        }

        std::unordered_set<Tiled2dMapVectorTileInfo> tilesToRemove;
        for (const auto &tileEntry : tileSet) {
            if (currentTileInfos.count(tileEntry) == 0)
                tilesToRemove.insert(tileEntry);
        }

        if (tilesToAdd.empty() && tilesToRemove.empty()) return;

        std::unordered_map<Tiled2dMapTileInfo, std::shared_ptr<PolygonMaskObject>> newTileMasks;

        auto const &graphicsObjectFactory = mapInterface->getGraphicsObjectFactory();

        for (const auto &tile : tilesToAdd) {
            if (!vectorTileSource->isTileVisible(tile.tileInfo)) continue;
            auto const &layerFeatureMap = tile.layerFeatureMap;

            if (newTileMasks.count(tile.tileInfo) == 0) {
                const auto &tileMask = std::make_shared<PolygonMaskObject>(graphicsObjectFactory,
                                                                        coordinateConverterHelper);

                tileMask->setPolygon(tile.mask);
                newTileMasks[tile.tileInfo] = tileMask;
            }

            for (auto it = layerFeatureMap->begin() ; it != layerFeatureMap->end(); it++ ) {
                std::string sourceLayerName = it->first;
                if (sourceLayerMap.count(sourceLayerName) > 0) {
                    for (const auto &subLayer : sourceLayerMap[sourceLayerName]) {
                        {
                            std::lock_guard<std::recursive_mutex> tilesReadyCountLock(tilesReadyCountMutex);
                            tilesReadyCount[tile.tileInfo] += 1;
                        }
                        subLayer->updateTileData(tile.tileInfo, newTileMasks[tile.tileInfo]->getPolygonObject()->asMaskingObject(), it->second);
                    }
                }
            }
            tileSet.insert(tile);
        }

        std::vector<const std::shared_ptr<MaskingObjectInterface>> toSetupMaskObjects;
        std::vector<const std::shared_ptr<MaskingObjectInterface>> toClearMaskObjects;

        for (const auto &newMaskEntry : newTileMasks) {
            if (tileMaskMap.count(newMaskEntry.first) > 0) {
                toClearMaskObjects.emplace_back(tileMaskMap[newMaskEntry.first]->getPolygonObject()->asMaskingObject());
            } 
            tileMaskMap[newMaskEntry.first] = newMaskEntry.second;
            toSetupMaskObjects.emplace_back(newMaskEntry.second->getPolygonObject()->asMaskingObject());
        }

        const auto &currentViewBounds = vectorTileSource->getCurrentViewBounds();

        for (const auto &tile : tilesToRemove) {
            for (const auto &sourceSubLayerPair : sourceLayerMap) {
                for (const auto &subLayer : sourceSubLayerPair.second) {
                    subLayer->clearTileData(tile.tileInfo);
                }
            }
            if (tileMaskMap.count(tile.tileInfo) > 0) {
                toClearMaskObjects.emplace_back(tileMaskMap[tile.tileInfo]->getPolygonObject()->asMaskingObject());
                tileMaskMap.erase(tile.tileInfo);
            }
            {
                std::lock_guard<std::recursive_mutex> tilesReadyLock(tilesReadyMutex);
                tilesReady.erase(tile.tileInfo);
            }
            {
                std::lock_guard<std::recursive_mutex> tilesReadyCountLock(tilesReadyCountMutex);
                tilesReadyCount.erase(tile.tileInfo);
            }
            tileSet.erase(tile);
        }

        if (!(toSetupMaskObjects.empty() && toClearMaskObjects.empty())) {
            std::weak_ptr<Tiled2dMapVectorLayer> weakSelfPtr = std::dynamic_pointer_cast<Tiled2dMapVectorLayer>(shared_from_this());
            mapInterface->getScheduler()->addTask(std::make_shared<LambdaTask>(
                    TaskConfig("VectorTile_masks_update", 0, TaskPriority::NORMAL, ExecutionEnvironment::GRAPHICS),
                    [weakSelfPtr, toSetupMaskObjects, toClearMaskObjects] {
                        auto selfPtr = weakSelfPtr.lock();
                        if (selfPtr) {
                            selfPtr->updateMaskObjects(toClearMaskObjects);
                        }
                    }));
        }

    }

    mapInterface->invalidate();
}

void Tiled2dMapVectorLayer::updateMaskObjects(const std::vector<const std::shared_ptr<MaskingObjectInterface>> &obsoleteMaskObjects) {
    auto mapInterface = this->mapInterface;
    auto renderingContext = mapInterface ? mapInterface->getRenderingContext() : nullptr;
    if (!renderingContext) return;
    std::lock_guard<std::recursive_mutex> overlayLock(updateMutex);
    for (const auto &entry : tileSet) {
        const auto &mask = tileMaskMap.at(entry.tileInfo);
        mask->setPolygon(entry.mask);
        mask->getPolygonObject()->asGraphicsObject()->setup(renderingContext);
    }
    for (const auto &mask : obsoleteMaskObjects) {
        const auto &object = mask->asGraphicsObject();
        if (object->isReady()) object->clear();
    }
}


void Tiled2dMapVectorLayer::tileIsReady(const Tiled2dMapTileInfo &tile) {
    {
        std::lock_guard<std::recursive_mutex> tilesReadyLock(tilesReadyMutex);
        if (tilesReady.count(tile) > 0) return;
    }
    bool isCompletlyReady = false;
    {
        std::lock_guard<std::recursive_mutex> tilesReadyCountLock(tilesReadyCountMutex);
        tilesReadyCount[tile] -= 1;
        if (tilesReadyCount.at(tile) == 0) {
            tilesReadyCount.erase(tile);
            {
                std::lock_guard<std::recursive_mutex> tilesReadyLock(tilesReadyMutex);
                tilesReady.insert(tile);
            }
            isCompletlyReady = true;
        }
    }

    if (isCompletlyReady) {
        vectorTileSource->setTileReady(tile);
    }

}

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
#include "Tiled2dMapVectorRasterSubLayer.h"
#include "Tiled2dMapVectorLayerParserHelper.h"
#include "RasterVectorLayerDescription.h"
#include "LineVectorLayerDescription.h"
#include "PolygonVectorLayerDescription.h"
#include "SymbolVectorLayerDescription.h"
#include "BackgroundVectorLayerDescription.h"
#include "Tiled2dMapVectorLineSubLayer.h"
#include "Tiled2dMapVectorPolygonSubLayer.h"
#include "VectorTileGeometryHandler.h"
#include "Tiled2dMapVectorSymbolSubLayer.h"
#include "Tiled2dMapVectorBackgroundSubLayer.h"
#include "Polygon2dInterface.h"
#include "MapCamera2dInterface.h"
#include "QuadMaskObject.h"
#include "PolygonMaskObject.h"
#include "CoordinatesUtil.h"
#include "RenderPass.h"
#include "OBB2D.h"
#include "DataLoaderResult.h"
#include "DataHolderInterface.h"
#include "TextureLoaderResult.h"
#include "PolygonCompare.h"
#include "LoaderHelper.h"


Tiled2dMapVectorLayer::Tiled2dMapVectorLayer(const std::string &layerName,
                                             const std::string &remoteStyleJsonUrl,
                                             const std::vector<std::shared_ptr<::LoaderInterface>> &loaders,
                                             const std::shared_ptr<::FontLoaderInterface> &fontLoader,
                                             double dpFactor) :
        Tiled2dMapLayer(),
        layerName(layerName),
        remoteStyleJsonUrl(remoteStyleJsonUrl),
        loaders(loaders),
        fontLoader(fontLoader),
        dpFactor(dpFactor),
        sublayers() {}


Tiled2dMapVectorLayer::Tiled2dMapVectorLayer(const std::string &layerName,
                                             const std::string &remoteStyleJsonUrl,
                                             const std::string &fallbackStyleJsonString,
                                             const std::vector<std::shared_ptr<::LoaderInterface>> &loaders,
                                             const std::shared_ptr<::FontLoaderInterface> &fontLoader,
                                             double dpFactor) :
        Tiled2dMapLayer(),
        layerName(layerName),
        remoteStyleJsonUrl(remoteStyleJsonUrl),
        fallbackStyleJsonString(fallbackStyleJsonString),
        loaders(loaders),
        fontLoader(fontLoader),

        dpFactor(dpFactor),
        sublayers() {
        }

Tiled2dMapVectorLayer::Tiled2dMapVectorLayer(const std::string &layerName, const std::shared_ptr<VectorMapDescription> &mapDescription,
                                             const std::vector<std::shared_ptr<::LoaderInterface>> &loaders,
                                             const std::shared_ptr<::FontLoaderInterface> &fontLoader) :
        Tiled2dMapLayer(),
        layerName(layerName),
        loaders(loaders),
        fontLoader(fontLoader),
        sublayers() {
    setMapDescription(mapDescription);
}

Tiled2dMapVectorLayer::Tiled2dMapVectorLayer(const std::string &layerName,
                                             const std::vector<std::shared_ptr<::LoaderInterface>> &loaders,
                                             const std::shared_ptr<::FontLoaderInterface> &fontLoader) :
        Tiled2dMapLayer(),
        layerName(layerName),
        loaders(loaders),
        fontLoader(fontLoader),
        sublayers() {
}

void Tiled2dMapVectorLayer::scheduleStyleJsonLoading() {
    isLoadingStyleJson = true;
    std::weak_ptr<Tiled2dMapVectorLayer> weakSelfPtr = std::dynamic_pointer_cast<Tiled2dMapVectorLayer>(shared_from_this());
    mapInterface->getScheduler()->addTask(std::make_shared<LambdaTask>(
            TaskConfig("VectorTile_loadStyleJson", 0, TaskPriority::NORMAL, ExecutionEnvironment::IO),
            [weakSelfPtr] {
                auto selfPtr = weakSelfPtr.lock();
                if (selfPtr) {
                    auto layerError = selfPtr->loadStyleJson();
                    if (selfPtr->errorManager) {
                        if (auto error = layerError) {
                            selfPtr->errorManager->addTiledLayerError(*error);
                        } else {
                            if (selfPtr->remoteStyleJsonUrl.has_value()) {
                                selfPtr->errorManager->removeError(*selfPtr->remoteStyleJsonUrl);
                            } else {
                                selfPtr->errorManager->removeError(selfPtr->layerName);
                            }
                        }
                    }

                    selfPtr->isLoadingStyleJson = false;
                }
            }));
}

std::optional<TiledLayerError> Tiled2dMapVectorLayer::loadStyleJson() {
    auto error = loadStyleJsonRemotely();
    if (error.has_value()) {
        if (auto json = fallbackStyleJsonString) {
            return loadStyleJsonLocally(*json);
        } else {
            return error;
        }
    } else {
        return std::nullopt;
    }
}

std::optional<TiledLayerError> Tiled2dMapVectorLayer::loadStyleJsonRemotely() {
    auto remoteStyleJsonUrl = this->remoteStyleJsonUrl;
    auto dpFactor = this->dpFactor;
    if (!remoteStyleJsonUrl.has_value() || !dpFactor.has_value()) {
        return std::nullopt;
    }
    auto parseResult = Tiled2dMapVectorLayerParserHelper::parseStyleJsonFromUrl(layerName, *remoteStyleJsonUrl, *dpFactor, loaders);
    if (parseResult.status == LoaderStatus::OK) {
        setMapDescription(parseResult.mapDescription);
        return std::nullopt;
    } else {
        return TiledLayerError(parseResult.status, parseResult.errorCode, layerName,
                                               *remoteStyleJsonUrl, parseResult.status != LoaderStatus::ERROR_404 ||
                                                               parseResult.status != LoaderStatus::ERROR_400, std::nullopt);
    }
}

std::optional<TiledLayerError> Tiled2dMapVectorLayer::loadStyleJsonLocally(std::string styleJsonString) {
    auto dpFactor = this->dpFactor;
    if (!dpFactor.has_value()) {
        return std::nullopt;
    }

    auto parseResult = Tiled2dMapVectorLayerParserHelper::parseStyleJsonFromString(layerName, styleJsonString, *dpFactor, loaders);

    if (parseResult.status == LoaderStatus::OK) {
        setMapDescription(parseResult.mapDescription);
        return std::nullopt;
    } else {
        return TiledLayerError(parseResult.status, parseResult.errorCode, layerName,
                               layerName, false, std::nullopt);
    }
}

std::shared_ptr<LayerInterface> Tiled2dMapVectorLayer::getLayerForDescription(const std::shared_ptr<VectorLayerDescription> &layerDescription) {
    switch (layerDescription->getType()) {
        case VectorLayerType::background: {
            auto backgroundDesc = std::static_pointer_cast<BackgroundVectorLayerDescription>(layerDescription);
            return std::make_shared<Tiled2dMapVectorBackgroundSubLayer>(backgroundDesc);
        }
        case VectorLayerType::raster: {
            auto rasterDesc = std::static_pointer_cast<RasterVectorLayerDescription>(layerDescription);
            return std::make_shared<Tiled2dMapVectorRasterSubLayer>(rasterDesc, loaders);
        }
        case VectorLayerType::line: {
            auto lineDesc = std::static_pointer_cast<LineVectorLayerDescription>(layerDescription);
            return std::make_shared<Tiled2dMapVectorLineSubLayer>(lineDesc);
        }
        case VectorLayerType::polygon: {
            auto polyDesc = std::static_pointer_cast<PolygonVectorLayerDescription>(layerDescription);
            return std::make_shared<Tiled2dMapVectorPolygonSubLayer>(polyDesc);
        }
        case VectorLayerType::symbol: {
            auto symbolDesc = std::static_pointer_cast<SymbolVectorLayerDescription>(layerDescription);
            return std::make_shared<Tiled2dMapVectorSymbolSubLayer>(fontLoader, symbolDesc);
        }
    }
    return nullptr;
}

std::shared_ptr<Tiled2dMapLayerConfig>
Tiled2dMapVectorLayer::getLayerConfig(const std::shared_ptr<VectorMapSourceDescription> &source) {
    return std::make_shared<Tiled2dMapVectorLayerConfig>(source);
}

void Tiled2dMapVectorLayer::setMapDescription(const std::shared_ptr<VectorMapDescription> &mapDescription) {
    std::vector<std::shared_ptr<LayerInterface>> newSublayers;
    std::unordered_map<std::string, std::unordered_map<std::string, std::vector<std::shared_ptr<Tiled2dMapVectorSubLayer>>>> newSourceLayerMap;

    for (auto const &layerDesc: mapDescription->layers) {
        std::shared_ptr<LayerInterface> layer = getLayerForDescription(layerDesc);
        if (!layer) {
            continue;
        }
        newSublayers.push_back(layer);
        switch (layerDesc->getType()) {
            case VectorLayerType::background:
            case VectorLayerType::raster: {
                break;
            }
            case VectorLayerType::custom:
            case VectorLayerType::line:
            case VectorLayerType::polygon:
            case VectorLayerType::symbol: {
                auto subLayer = std::dynamic_pointer_cast<Tiled2dMapVectorSubLayer>(layer);
                newSourceLayerMap[layerDesc->source][layerDesc->sourceId].push_back(subLayer);
                break;
            }
        }
    }

    sourceLayerMap = newSourceLayerMap;
    {
        std::lock_guard<std::recursive_mutex> lock(sublayerMutex);
        std::vector<std::shared_ptr<LayerInterface>> oldSublayers = sublayers;
        sublayers.clear();
        for (const auto &layer: oldSublayers) {
            layer->onRemoved();
        }

        this->mapDescription = mapDescription;
        this->layerConfigs.clear();

        for (auto const &source: mapDescription->vectorSources) {
            layerConfigs[source->identifier] = getLayerConfig(source);
        }

        initializeVectorLayer(newSublayers);
        sublayers = newSublayers;
    }
}

void Tiled2dMapVectorLayer::initializeVectorLayer(const std::vector<std::shared_ptr<LayerInterface>> &newSublayers) {
    auto mapInterface = this->mapInterface;
    if (!mapInterface) {
        return;
    }

    std::unordered_map<std::string, std::unordered_set<std::string>> layersToDecode;

    for (auto const& [source, map] : sourceLayerMap)
    {
        for (auto const& [sourceLayer, layer] : map)
        {
            layersToDecode[source].insert(sourceLayer);
        }
    }

    vectorTileSource = std::make_shared<Tiled2dMapVectorSource>(mapInterface->getMapConfig(),
                                                                layerConfigs,
                                                                mapInterface->getCoordinateConverterHelper(),
                                                                mapInterface->getScheduler(),
                                                                loaders,
                                                                shared_from_this(),
                                                                layersToDecode,
                                                                mapInterface->getCamera()->getScreenDensityPpi());

    setSourceInterface(vectorTileSource);
    Tiled2dMapLayer::onAdded(mapInterface);
    mapInterface->getTouchHandler()->addListener(shared_from_this());

    if (mapDescription->spriteBaseUrl) {
        loadSpriteData();
    }

    {
        std::lock_guard<std::recursive_mutex> lock(sublayerMutex);
        for (auto const &layer: sublayers) {
            layer->onAdded(mapInterface);
            auto vectorSubLayer = std::dynamic_pointer_cast<Tiled2dMapVectorSubLayer>(layer);
            if (vectorSubLayer) {
                vectorSubLayer->setTilesReadyDelegate(
                        std::dynamic_pointer_cast<Tiled2dMapVectorLayerReadyInterface>(shared_from_this()));
            }
        }
    }
    for (const auto &newLayer : newSublayers) {
        newLayer->onAdded(mapInterface);
        auto vectorSubLayer = std::dynamic_pointer_cast<Tiled2dMapVectorSubLayer>(newLayer);
        if (vectorSubLayer) {
            vectorSubLayer->setTilesReadyDelegate(
                    std::dynamic_pointer_cast<Tiled2dMapVectorLayerReadyInterface>(shared_from_this()));
            vectorSubLayer->setSelectionDelegate(selectionDelegate);
        }
    }

    if (isResumed) {
        resume();
    }

}

std::shared_ptr<::LayerInterface> Tiled2dMapVectorLayer::asLayerInterface() {
    return shared_from_this();
}

void Tiled2dMapVectorLayer::update() {
    std::lock_guard<std::recursive_mutex> lock(sublayerMutex);
    bool needsCollisionDetection = false;
    for (auto it = sublayers.rbegin(); it != sublayers.rend(); ++it) {
        if (auto symbolLayer = std::dynamic_pointer_cast<Tiled2dMapVectorSymbolSubLayer>(*it)) {
            if (symbolLayer->isDirty()) {
                needsCollisionDetection = true;
                break;
            }
        }
    }
    if (needsCollisionDetection) {
        std::vector<OBB2D> placements;
        for (auto it = sublayers.rbegin(); it != sublayers.rend(); ++it)
        {
            if (auto symbolLayer = std::dynamic_pointer_cast<Tiled2dMapVectorSymbolSubLayer>(*it)) {
                symbolLayer->collisionDetection(placements);
            }
        }
    }
    
    for (auto const &layer: sublayers) {
        layer->update();
    }
}

std::vector<std::shared_ptr<::RenderPassInterface>> Tiled2dMapVectorLayer::buildRenderPasses() {
    std::scoped_lock<std::recursive_mutex, std::recursive_mutex> lock(tilesReadyMutex, sublayerMutex);
    std::vector<std::shared_ptr<RenderPassInterface>> newPasses;
    for (const auto &layer: sublayers) {
        std::vector<std::shared_ptr<RenderPassInterface>> sublayerPasses;
        auto const &castedPtr = std::dynamic_pointer_cast<Tiled2dMapVectorSubLayer>(layer);
        if (castedPtr != nullptr) {
            sublayerPasses = castedPtr->buildRenderPasses(tilesReady);
        } else {
            sublayerPasses = layer->buildRenderPasses();
        }
        newPasses.insert(newPasses.end(), sublayerPasses.begin(), sublayerPasses.end());
    }

    return newPasses;
}

void Tiled2dMapVectorLayer::onAdded(const std::shared_ptr<::MapInterface> &mapInterface) {
    this->mapInterface = mapInterface;
    setSelectionDelegate(std::dynamic_pointer_cast<Tiled2dMapVectorLayerSelectionInterface>(shared_from_this()));

    if (layerConfigs.empty()) {
        scheduleStyleJsonLoading();
        return;
    }

    initializeVectorLayer({});
}

void Tiled2dMapVectorLayer::onRemoved() {
    auto const mapInterface = this->mapInterface;
    if (mapInterface) {
        mapInterface->getTouchHandler()->removeListener(shared_from_this());
    }
    Tiled2dMapLayer::onRemoved();
    pause();

    std::lock_guard<std::recursive_mutex> lock(sublayerMutex);
    for (auto const &layer: sublayers) {
        layer->onRemoved();
    }
}

void Tiled2dMapVectorLayer::pause() {
    isResumed = false;

    if (vectorTileSource) {
        vectorTileSource->pause();
    }

    {
        std::lock_guard<std::recursive_mutex> overlayLock(tileMaskMapMutex);
        for (const auto &tileMask : tileMaskMap) {
            if (tileMask.second.maskObject &&
                tileMask.second.maskObject->getPolygonObject()->asGraphicsObject()->isReady()) {
                tileMask.second.maskObject->getPolygonObject()->asGraphicsObject()->clear();
            }
        }
    }
    {
        std::lock_guard<std::recursive_mutex> lock(sublayerMutex);
        for (auto const &layer: sublayers) {
            layer->pause();
        }
    }

    {
        std::lock_guard<std::recursive_mutex> tilesReadyLock(tilesReadyMutex);
        tilesReady.clear();
        tilesReadyCount.clear();
    }
}

void Tiled2dMapVectorLayer::resume() {
    isResumed = true;

    if (!vectorTileSource) {
        return;
    }
    vectorTileSource->resume();
    const auto &context = mapInterface->getRenderingContext();
    {
        std::lock_guard<std::recursive_mutex> overlayLock(tileMaskMapMutex);
        for (const auto &tileMask : tileMaskMap) {
            if (tileMask.second.maskObject &&
                !tileMask.second.maskObject->getPolygonObject()->asGraphicsObject()->isReady()) {
                tileMask.second.maskObject->getPolygonObject()->asMaskingObject()->asGraphicsObject()->setup(context);
            }
        }
    }

    {
        std::lock_guard<std::recursive_mutex> tilesReadyCountLock(tilesReadyMutex);
        for (const auto &tile: tileSet) {
            tilesReady.insert(tile.tileInfo);
            vectorTileSource->setTileReady(tile.tileInfo);
        }
    }

    {
        std::lock_guard<std::recursive_mutex> lock(sublayerMutex);
        for (auto const &layer: sublayers) {
            layer->resume();
        }
    }
}

void Tiled2dMapVectorLayer::setAlpha(float alpha) {
    if (this->alpha == alpha) {
        return;
    }
    this->alpha = alpha;
    {
        std::lock_guard<std::recursive_mutex> lock(sublayerMutex);
        for (auto const &sublayer: sublayers) {
            sublayer->setAlpha(alpha);
        }
    }

    if (mapInterface)
        mapInterface->invalidate();
}

float Tiled2dMapVectorLayer::getAlpha() { return alpha; }


void Tiled2dMapVectorLayer::forceReload() {
    if (!isLoadingStyleJson && remoteStyleJsonUrl.has_value() && !mapDescription && !vectorTileSource) {
        scheduleStyleJsonLoading();
        return;
    }
    Tiled2dMapLayer::forceReload();
}


void Tiled2dMapVectorLayer::onTilesUpdated() {

    auto lockSelfPtr = std::static_pointer_cast<Tiled2dMapVectorLayer>(shared_from_this());
    auto mapInterface = lockSelfPtr ? lockSelfPtr->mapInterface : nullptr;
    {

        auto graphicsFactory = mapInterface ? mapInterface->getGraphicsObjectFactory() : nullptr;
        auto coordinateConverterHelper = mapInterface ? mapInterface->getCoordinateConverterHelper() : nullptr;
        auto shaderFactory = mapInterface ? mapInterface->getShaderFactory() : nullptr;
        if (!graphicsFactory || !shaderFactory) {
            return;
        }

        //make sure only one tile update is run at a time
        std::lock_guard<std::recursive_mutex> updateLock(tileUpdateMutex);

        auto const &currentTileInfos = vectorTileSource->getCurrentTiles();

        std::unordered_set<Tiled2dMapVectorTileInfo> tilesToAdd;
        std::unordered_set<Tiled2dMapVectorTileInfo> tilesToKeep;

        std::unordered_set<Tiled2dMapVectorTileInfo> tilesToRemove;

        {
            std::lock_guard<std::recursive_mutex> overlayLock(tileSetMutex);
            for (const auto &vectorTileInfo : currentTileInfos) {
                if (tileSet.count(vectorTileInfo) == 0) {
                    tilesToAdd.insert(vectorTileInfo);
                } else {
                    tilesToKeep.insert(vectorTileInfo);
                }
            }

            for (const auto &tileEntry : tileSet) {
                if (currentTileInfos.count(tileEntry) == 0)
                    tilesToRemove.insert(tileEntry);
            }
        }



        std::unordered_map<Tiled2dMapTileInfo, Tiled2dMapLayerMaskWrapper> newTileMasks;
        for (const auto &tileEntry : tilesToKeep) {

            size_t existingPolygonHash;
            {
                std::lock_guard<std::recursive_mutex> lock(tileMaskMapMutex);
                existingPolygonHash = tileMaskMap.at(tileEntry.tileInfo).polygonHash;
            }
            const size_t hash = std::hash<std::vector<::PolygonCoord>>()(tileEntry.masks);

            if (hash != existingPolygonHash) {

                const auto &tileMask = std::make_shared<PolygonMaskObject>(graphicsFactory,
                                                                           coordinateConverterHelper);

                tileMask->setPolygons(tileEntry.masks);

                newTileMasks[tileEntry.tileInfo] = Tiled2dMapLayerMaskWrapper(tileMask, hash);
            }
        }

        if (tilesToAdd.empty() && tilesToRemove.empty() && newTileMasks.empty()) return;



        for (const auto &tile : tilesToAdd) {
            if (!vectorTileSource->isTileVisible(tile.tileInfo)) continue;

            if (newTileMasks.count(tile.tileInfo) == 0) {
                const auto &tileMask = std::make_shared<PolygonMaskObject>(graphicsFactory,
                                                                        coordinateConverterHelper);

                tileMask->setPolygons(tile.masks);

                const size_t &hash = std::hash<std::vector<::PolygonCoord>>()(tile.masks);

                newTileMasks[tile.tileInfo] = Tiled2dMapLayerMaskWrapper(tileMask, hash);
            }



            {
                std::lock_guard<std::recursive_mutex> lock(sourceLayerMapMutex);
                for (auto const &[source, layerFeatureMap]: tile.layerFeatureMaps) {
                    for (auto it = layerFeatureMap->begin(); it != layerFeatureMap->end(); it++) {
                        auto sourceLayerMapEntry = sourceLayerMap.at(source).find(it->first);
                        if (sourceLayerMapEntry != sourceLayerMap.at(source).end() && !sourceLayerMapEntry->second.empty()) {
                            for (const auto &subLayer : sourceLayerMapEntry->second) {
                                {
                                    std::lock_guard<std::recursive_mutex> tilesReadyCountLock(tilesReadyCountMutex);
                                    tilesReadyCount[tile.tileInfo] += 1;
                                }

                                std::weak_ptr<Tiled2dMapVectorLayer> weakSelfPtr = std::dynamic_pointer_cast<Tiled2dMapVectorLayer>(shared_from_this());
                                auto const polygonObject = newTileMasks[tile.tileInfo].maskObject->getPolygonObject()->asMaskingObject();
                                auto const &features = it->second;
                                mapInterface->getScheduler()->addTask(std::make_shared<LambdaTask>(
                                                                                                   TaskConfig("VectorTile_onTilesUpdated_" + it->first, 0, TaskPriority::NORMAL, ExecutionEnvironment::COMPUTATION),
                                                                                                   [weakSelfPtr, subLayer, tile, polygonObject, &features] {
                                                                                                       auto selfPtr = weakSelfPtr.lock();
                                                                                                       if (selfPtr) {
                                                                                                           subLayer->updateTileData(tile.tileInfo, polygonObject, features);
                                                                                                       }
                                                                                                   }));
                            }
                        }
                    }
                }
            }

            {

                std::lock_guard<std::recursive_mutex> lock(tileSetMutex);
                tileSet.insert(tile);

            }
        }

        std::vector<const std::shared_ptr<MaskingObjectInterface>> toClearMaskObjects;


        for (const auto &newMaskEntry : newTileMasks) {
            auto oldIt = tileMaskMap.find(newMaskEntry.first);
            if (oldIt != tileMaskMap.end() && oldIt->second.maskObject) {
                toClearMaskObjects.emplace_back(oldIt->second.maskObject->getPolygonObject()->asMaskingObject());
            }
            tileMaskMap[newMaskEntry.first] = newMaskEntry.second;
        }

        const auto &currentViewBounds = vectorTileSource->getCurrentViewBounds();

        for (const auto &tile : tilesToRemove) {
            for (const auto &[source, sourceLayersMap] : sourceLayerMap) {
                for (const auto &sourceSubLayerPair : sourceLayersMap) {
                    for (const auto &subLayer : sourceSubLayerPair.second) {
                        subLayer->clearTileData(tile.tileInfo);
                    }
                }
            }
            auto maskIt = tileMaskMap.find(tile.tileInfo);
            if (maskIt != tileMaskMap.end() && maskIt->second.maskObject) {
                toClearMaskObjects.emplace_back(maskIt->second.maskObject->getPolygonObject()->asMaskingObject());
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

        if (!(newTileMasks.empty() && toClearMaskObjects.empty())) {
            std::weak_ptr<Tiled2dMapVectorLayer> weakSelfPtr = std::dynamic_pointer_cast<Tiled2dMapVectorLayer>(shared_from_this());
            mapInterface->getScheduler()->addTask(std::make_shared<LambdaTask>(
                    TaskConfig("VectorTile_masks_update", 0, TaskPriority::NORMAL, ExecutionEnvironment::GRAPHICS),
                    [weakSelfPtr, newTileMasks, toClearMaskObjects] {
                        auto selfPtr = weakSelfPtr.lock();
                        if (selfPtr) {
                            selfPtr->updateMaskObjects(newTileMasks, toClearMaskObjects);
                        }
                    }));
        }

    }

    mapInterface->invalidate();
}

void Tiled2dMapVectorLayer::updateMaskObjects(const std::unordered_map<Tiled2dMapTileInfo, Tiled2dMapLayerMaskWrapper> &toSetupMaskObject,
                                              const std::vector<const std::shared_ptr<MaskingObjectInterface>> &obsoleteMaskObjects) {
    auto mapInterface = this->mapInterface;
    auto renderingContext = mapInterface ? mapInterface->getRenderingContext() : nullptr;
    if (!renderingContext) return;
    for (const auto &[tileInfo, wrapper] : toSetupMaskObject) {
        wrapper.maskObject->getPolygonObject()->asGraphicsObject()->setup(renderingContext);
        {
            std::lock_guard<std::recursive_mutex> lock(tileMaskMapMutex);
            tileMaskMap[tileInfo] = wrapper;
        }

        {
            std::lock_guard<std::recursive_mutex> lock(sublayerMutex);
            for (auto const &layer: sublayers) {
                if (auto subLayer = std::dynamic_pointer_cast<Tiled2dMapVectorSubLayer>(layer)) {
                    subLayer->updateTileMask(tileInfo, wrapper.maskObject->getPolygonObject()->asMaskingObject());
                }
            }
        }
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
    bool isCompletelyReady = false;
    {
        std::lock_guard<std::recursive_mutex> tilesReadyCountLock(tilesReadyCountMutex);
        tilesReadyCount[tile] -= 1;
        if (tilesReadyCount.at(tile) == 0) {
            tilesReadyCount.erase(tile);
            {
                std::lock_guard<std::recursive_mutex> tilesReadyLock(tilesReadyMutex);
                tilesReady.insert(tile);
            }
            isCompletelyReady = true;
        }
    }

    if (isCompletelyReady) {
        vectorTileSource->setTileReady(tile);
    }

}

void Tiled2dMapVectorLayer::loadSpriteData() {
    auto lockSelfPtr = shared_from_this();
    auto mapInterface = this->mapInterface;
    auto camera = mapInterface ? mapInterface->getCamera() : nullptr;
    auto scheduler = mapInterface ? mapInterface->getScheduler() : nullptr;
    if (!camera || !scheduler) {
        return;
    }

    bool scale2x = camera->getScreenDensityPpi() >= 320.0;
    std::stringstream ssTexture;
    ssTexture << *mapDescription->spriteBaseUrl << (scale2x ? "@2x" : "") << ".png";
    std::string urlTexture = ssTexture.str();
    std::stringstream ssData;
    ssData << *mapDescription->spriteBaseUrl << (scale2x ? "@2x" : "") << ".json";
    std::string urlData = ssData.str();

    std::weak_ptr<Tiled2dMapVectorLayer> weakSelfPtr =
    std::dynamic_pointer_cast<Tiled2dMapVectorLayer>(shared_from_this());
    scheduler->addTask(std::make_shared<LambdaTask>(
                                                    TaskConfig("Tiled2dMapVectorLayer_loadSpriteData",
                                                               0,
                                                               TaskPriority::NORMAL,
                                                               ExecutionEnvironment::IO),
                                                    [weakSelfPtr, urlTexture, urlData] {
                                                        auto selfPtr = weakSelfPtr.lock();
                                                        if (selfPtr) {
                                                            auto dataResult = LoaderHelper::loadData(urlData, std::nullopt, selfPtr->loaders);
                                                            auto textureResult = LoaderHelper::loadTexture(urlTexture, std::nullopt, selfPtr->loaders);
                                                            if (dataResult.status == LoaderStatus::OK && textureResult.status == LoaderStatus::OK) {
                                                                auto data = dataResult.data->getData();
                                                                auto string = std::string((char*)data.data(), data.size());
                                                                nlohmann::json json;
                                                                try
                                                                {
                                                                    json = nlohmann::json::parse(string);
                                                                }
                                                                catch (nlohmann::json::parse_error& ex)
                                                                {
                                                                    return;
                                                                }

                                                                std::unordered_map<std::string, SpriteDesc> sprites;

                                                                for (auto& [key, val] : json.items())
                                                                {
                                                                    sprites.insert({key, val.get<::SpriteDesc>()});
                                                                }

                                                                auto spriteData = std::make_shared<SpriteData>(sprites);
                                                                auto spriteTexture = textureResult.data;

                                                                {
                                                                    std::lock_guard<std::recursive_mutex> lock(selfPtr->sublayerMutex);
                                                                    for (auto const &layer: selfPtr->sublayers) {
                                                                        if (auto symbolLayer = std::dynamic_pointer_cast<Tiled2dMapVectorSymbolSubLayer>(
                                                                                layer)) {
                                                                            symbolLayer->setSprites(spriteTexture, spriteData);
                                                                        }
                                                                    }
                                                                }

                                                            } else {
                                                                //TODO: Error handling
                                                            }
                                                        }
                                                    }
                                                    )
                       );
}

void Tiled2dMapVectorLayer::setScissorRect(const std::optional<::RectI> &scissorRect) {
    {
        std::lock_guard<std::recursive_mutex> lock(sublayerMutex);
        for (auto const &sublayer: sublayers) {
            sublayer->setScissorRect(scissorRect);
        }
    }
    auto mapInterface = this->mapInterface;
    if (mapInterface) {
        mapInterface->invalidate();
    }
}

void Tiled2dMapVectorLayer::setSelectionDelegate(const std::weak_ptr<Tiled2dMapVectorLayerSelectionInterface> selectionDelegate) {
    this->selectionDelegate = selectionDelegate;
    {
        std::lock_guard<std::recursive_mutex> lock(sublayerMutex);
        for (auto const &sublayer: sublayers) {
            auto vectorSubLayer = std::dynamic_pointer_cast<Tiled2dMapVectorSubLayer>(sublayer);
            if (vectorSubLayer) {
                vectorSubLayer->setSelectionDelegate(selectionDelegate);
            }
        }
    }
}

void Tiled2dMapVectorLayer::setSelectedFeatureIdentfier(std::optional<int64_t> identifier) {
    {
        std::lock_guard<std::recursive_mutex> lock(sublayerMutex);
        for (auto const &sublayer: sublayers) {
            auto vectorSubLayer = std::dynamic_pointer_cast<Tiled2dMapVectorSubLayer>(sublayer);
            if (vectorSubLayer) {
                vectorSubLayer->setSelectedFeatureIdentfier(identifier);
            }
        }
    }
}

void Tiled2dMapVectorLayer::updateLayerDescription(std::shared_ptr<VectorLayerDescription> layerDescription) {
    auto mapInterface = this->mapInterface;
    std::shared_ptr<LayerInterface> layer = getLayerForDescription(layerDescription);
    if (!layer) {
        return;
    }

    auto newVectorSubLayer = std::dynamic_pointer_cast<Tiled2dMapVectorSubLayer>(layer);

    {
        std::lock_guard<std::recursive_mutex> lock(sublayerMutex);

        std::replace_if(std::begin(sublayers), std::end(sublayers), [&layerDescription](const std::shared_ptr<LayerInterface> &sublayer) {
            auto vectorSubLayer = std::dynamic_pointer_cast<Tiled2dMapVectorSubLayer>(sublayer);
            if (vectorSubLayer && vectorSubLayer->getLayerDescriptionIdentifier() == layerDescription->identifier) {
                vectorSubLayer->onRemoved();
                return true;
            }
            return false;
        }, layer);

        if (newVectorSubLayer && mapInterface) {
            newVectorSubLayer->onAdded(mapInterface);
        }
    }
    if (newVectorSubLayer) {
        newVectorSubLayer->setTilesReadyDelegate(std::dynamic_pointer_cast<Tiled2dMapVectorLayerReadyInterface>(shared_from_this()));
        newVectorSubLayer->setSelectionDelegate(selectionDelegate);


        std::lock_guard<std::recursive_mutex> lock(sourceLayerMapMutex);

        std::replace_if(std::begin(sourceLayerMap[layerDescription->source][layerDescription->sourceId]), std::end(sourceLayerMap[layerDescription->source][layerDescription->sourceId]), [&layerDescription](const std::shared_ptr<Tiled2dMapVectorSubLayer> &sublayer) {
            return sublayer->getLayerDescriptionIdentifier() == layerDescription->identifier;
        },  newVectorSubLayer);

        std::lock_guard<std::recursive_mutex> updateLock(tileUpdateMutex);

        auto const &currentTileInfos = vectorTileSource->getCurrentTiles();

        for (auto const &tile: currentTileInfos) {
            {
                std::lock_guard<std::recursive_mutex> lock(sourceLayerMapMutex);
                for (auto const &[source, layerFeatureMap]: tile.layerFeatureMaps) {
                    if (source != layerDescription->source) continue;
                    for (auto it = layerFeatureMap->begin(); it != layerFeatureMap->end(); it++) {
                        if (it->first != layerDescription->sourceId) continue;

                        {
                            std::lock_guard<std::recursive_mutex> tilesReadyCountLock(tilesReadyCountMutex);
                            tilesReadyCount[tile.tileInfo] = 0;
                        }

                        std::weak_ptr<Tiled2dMapVectorLayer> weakSelfPtr = std::dynamic_pointer_cast<Tiled2dMapVectorLayer>(shared_from_this());
                        std::shared_ptr<MaskingObjectInterface> polygonObject;
                        {
                            std::lock_guard<std::recursive_mutex> overlayLock(tileMaskMapMutex);
                            if (tileMaskMap.count(tile.tileInfo) != 0) {
                                polygonObject = tileMaskMap[tile.tileInfo].maskObject->getPolygonObject()->asMaskingObject();
                            }
                        }
                        auto const &features = it->second;
                        mapInterface->getScheduler()->addTask(std::make_shared<LambdaTask>(
                                                                                           TaskConfig("VectorTile_updateLayerDescription_" + it->first, 0, TaskPriority::NORMAL, ExecutionEnvironment::COMPUTATION),
                                                                                           [weakSelfPtr, newVectorSubLayer, tile, polygonObject, &features] {
                                                                                               auto selfPtr = weakSelfPtr.lock();
                                                                                               if (selfPtr) {
                                                                                                   newVectorSubLayer->updateTileData(tile.tileInfo, polygonObject, features);
                                                                                               }
                                                                                           }));

                    }
                }
            }

        }


    }
}

std::optional<FeatureContext> Tiled2dMapVectorLayer::getFeatureContext(int64_t identifier) {
    auto const &currentTileInfos = vectorTileSource->getCurrentTiles();

    for (auto const &tile: currentTileInfos) {
        {
            for (auto const &[source, layerFeatureMap]: tile.layerFeatureMaps) {
                for (auto it = layerFeatureMap->begin(); it != layerFeatureMap->end(); it++) {
                    for (auto const &[featureContext, geometry]: it->second) {
                        if (featureContext.identifier == identifier) {
                            return featureContext;
                        }
                    }
                }
            }
        }
    }
    
    return std::nullopt;
}

std::shared_ptr<VectorLayerDescription> Tiled2dMapVectorLayer::getLayerDescriptionWithIdentifier(std::string identifier) {
    if (mapDescription) {
        for (auto const &layer: mapDescription->layers) {
            if (layer->identifier == identifier) {
                return layer;
            }
        }
    }
    return nullptr;
}

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
#include "TextureLoaderResult.h"
#include "PolygonCompare.h"
#include "LoaderHelper.h"

#include "Tiled2dMapVectorPolygonTile.h"


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
        tiles() {}


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
        tiles() {
        }

Tiled2dMapVectorLayer::Tiled2dMapVectorLayer(const std::string &layerName, const std::shared_ptr<VectorMapDescription> &mapDescription,
                                             const std::vector<std::shared_ptr<::LoaderInterface>> &loaders,
                                             const std::shared_ptr<::FontLoaderInterface> &fontLoader) :
        Tiled2dMapLayer(),
        layerName(layerName),
        loaders(loaders),
        fontLoader(fontLoader),
        tiles() {
    setMapDescription(mapDescription);
}

Tiled2dMapVectorLayer::Tiled2dMapVectorLayer(const std::string &layerName,
                                             const std::vector<std::shared_ptr<::LoaderInterface>> &loaders,
                                             const std::shared_ptr<::FontLoaderInterface> &fontLoader) :
        Tiled2dMapLayer(),
        layerName(layerName),
        loaders(loaders),
        fontLoader(fontLoader),
        tiles() {
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

std::shared_ptr<Tiled2dMapLayerConfig>
Tiled2dMapVectorLayer::getLayerConfig(const std::shared_ptr<VectorMapSourceDescription> &source) {
    return std::make_shared<Tiled2dMapVectorLayerConfig>(source);
}

void Tiled2dMapVectorLayer::setMapDescription(const std::shared_ptr<VectorMapDescription> &mapDescription) {

        this->mapDescription = mapDescription;
        this->layerConfigs.clear();

        for (auto const &source: mapDescription->vectorSources) {
            layerConfigs[source->identifier] = getLayerConfig(source);
        }
        initializeVectorLayer();
}

void Tiled2dMapVectorLayer::initializeVectorLayer() {
    auto mapInterface = this->mapInterface;
    if (!mapInterface) {
        return;
    }

    std::unordered_map<std::string, std::unordered_set<std::string>> layersToDecode;

    for (auto const& layerDesc : mapDescription->layers)
    {
        layersToDecode[layerDesc->source].insert(layerDesc->sourceId);
    }
    
    auto selfMailbox = std::make_shared<Mailbox>(mapInterface->getScheduler());
    auto castedMe = std::static_pointer_cast<Tiled2dMapVectorLayer>(shared_from_this());
    auto selfActor = WeakActor<Tiled2dMapVectorLayer>(selfMailbox, castedMe);

    auto mailbox = std::make_shared<Mailbox>(mapInterface->getScheduler());
    vectorTileSource.emplaceObject(mailbox, mapInterface->getMapConfig(),
                                   layerConfigs,
                                   mapInterface->getCoordinateConverterHelper(),
                                   mapInterface->getScheduler(),
                                   loaders,
                                   selfActor,
                                   layersToDecode,
                                   mapInterface->getCamera()->getScreenDensityPpi());

    setSourceInterface(vectorTileSource.weakActor<Tiled2dMapSourceInterface>());

    Tiled2dMapLayer::onAdded(mapInterface, layerIndex);
    mapInterface->getTouchHandler()->insertListener(std::dynamic_pointer_cast<TouchInterface>(shared_from_this()), layerIndex);

    if (mapDescription->spriteBaseUrl) {
        loadSpriteData();
    }

    if (isResumed) {
        vectorTileSource.message(&Tiled2dMapVectorSource::resume);
    }

}

std::shared_ptr<::LayerInterface> Tiled2dMapVectorLayer::asLayerInterface() {
    return shared_from_this();
}

void Tiled2dMapVectorLayer::update() {
    // TODO: update tiles
/*    std::lock_guard<std::recursive_mutex> lock(sublayerMutex);
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
    }*/

    std::lock_guard<std::recursive_mutex> lock(tilesMutex);
    for (const auto &[tileInfo, subTiles] : tiles) {
        for (const auto &tile: subTiles) {
            tile.syncAccess([](auto tile) {
                tile->update();
            });

            //tile.message(MailboxExecutionEnvironment::graphics, &Tiled2dMapVectorTile::update);
        }
    }
}

std::vector<std::shared_ptr<::RenderPassInterface>> Tiled2dMapVectorLayer::buildRenderPasses() {
    //std::scoped_lock<std::recursive_mutex, std::recursive_mutex> lock(tilesReadyMutex, sublayerMutex);
    std::vector<std::shared_ptr<RenderPassInterface>> newPasses;
/*    for (const auto &layer: sublayers) {
        std::vector<std::shared_ptr<RenderPassInterface>> sublayerPasses;
        auto const &castedPtr = std::dynamic_pointer_cast<Tiled2dMapVectorSubLayer>(layer);
        if (castedPtr != nullptr) {
            sublayerPasses = castedPtr->buildRenderPasses(tilesReady);
        } else {
            sublayerPasses = layer->buildRenderPasses();
        }
        newPasses.insert(newPasses.end(), sublayerPasses.begin(), sublayerPasses.end());
    }*/
    std::lock_guard<std::recursive_mutex> lock(tilesMutex);
    for (const auto &[tileInfo, subTiles] : tiles) {
        for (const auto &tile: subTiles) {
            const auto &tilePasses = tile.syncAccess([](auto tile) {
                return tile->buildRenderPasses();
            });
            newPasses.insert(newPasses.end(), tilePasses.begin(), tilePasses.end());
        }
    }
    return newPasses;
}

void Tiled2dMapVectorLayer::onAdded(const std::shared_ptr<::MapInterface> &mapInterface, int32_t layerIndex) {
    this->mapInterface = mapInterface;
    this->layerIndex = layerIndex;
    setSelectionDelegate(std::dynamic_pointer_cast<Tiled2dMapVectorLayerSelectionInterface>(shared_from_this()));

    if (layerConfigs.empty()) {
        scheduleStyleJsonLoading();
        return;
    }

    // TODO: really necessary?
    //s();
}

void Tiled2dMapVectorLayer::onRemoved() {
    auto const mapInterface = this->mapInterface;
    if (mapInterface) {
        mapInterface->getTouchHandler()->removeListener(std::dynamic_pointer_cast<TouchInterface>(shared_from_this()));
    }
    Tiled2dMapLayer::onRemoved();
    pause();
    {
        std::lock_guard<std::recursive_mutex> lock(tilesMutex);
        for (const auto &[tileInfo, subTiles] : tiles) {
            for (const auto &tile: subTiles) {
                tile.message(MailboxExecutionEnvironment::graphics, &Tiled2dMapVectorTile::clear);
            }
        }
        tiles.clear();
    }

    this->layerIndex = -1;
}

void Tiled2dMapVectorLayer::pause() {
    isResumed = false;

    if (vectorTileSource) {
        vectorTileSource.message(&Tiled2dMapVectorSource::pause);
    }

    {
        std::lock_guard<std::recursive_mutex> overlayLock(tileMaskMapMutex);
        for (const auto &tileMask : tileMaskMap) {
            if (tileMask.second.getGraphicsObject() &&
                tileMask.second.getGraphicsObject()->isReady()) {
                tileMask.second.getGraphicsObject()->clear();
            }
        }
    }
    {
        std::lock_guard<std::recursive_mutex> lock(tilesMutex);
        for (const auto &[tileInfo, subTiles] : tiles) {
            for (const auto &tile: subTiles) {
                tile.message(MailboxExecutionEnvironment::graphics, &Tiled2dMapVectorTile::clear);
            }
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
    vectorTileSource.message(&Tiled2dMapVectorSource::resume);
    const auto &context = mapInterface->getRenderingContext();
    {
        std::lock_guard<std::recursive_mutex> overlayLock(tileMaskMapMutex);
        for (const auto &tileMask : tileMaskMap) {
            if (tileMask.second.getGraphicsObject() && !tileMask.second.getGraphicsObject()->isReady()) {
                tileMask.second.getGraphicsObject()->setup(context);
            }
        }
    }

    {
        std::lock_guard<std::recursive_mutex> tilesReadyCountLock(tilesReadyMutex);
        for (const auto &[tileInfo, _]: tiles) {
            tilesReady.insert(tileInfo);
            vectorTileSource.message(&Tiled2dMapVectorSource::setTileReady, tileInfo);
        }
    }
    {
        std::lock_guard<std::recursive_mutex> lock(tilesMutex);
        for (const auto &[tileInfo, subTiles] : tiles) {
            for (const auto &tile: subTiles) {
                tile.message(MailboxExecutionEnvironment::graphics, &Tiled2dMapVectorTile::setup);
            }
        }
    }

}

void Tiled2dMapVectorLayer::setAlpha(float alpha) {
    if (this->alpha == alpha) {
        return;
    }
    this->alpha = alpha;
    {
        std::lock_guard<std::recursive_mutex> lock(tilesMutex);
        for (auto const &[tileInfo, subTiles]: tiles) {
            for (const auto &tile: subTiles) {
                tile.message(&Tiled2dMapVectorTile::setAlpha, alpha);
            }
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


void Tiled2dMapVectorLayer::onTilesUpdated(std::unordered_set<Tiled2dMapVectorTileInfo> currentTileInfos) {

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

        std::unordered_set<Tiled2dMapVectorTileInfo> tilesToAdd;
        std::unordered_set<Tiled2dMapVectorTileInfo> tilesToKeep;

        std::unordered_set<Tiled2dMapTileInfo> tilesToRemove;

        {
            std::lock_guard<std::recursive_mutex> lock(tilesMutex);

            for (const auto &vectorTileInfo: currentTileInfos) {
                if (tiles.count(vectorTileInfo.tileInfo) == 0) {
                    tilesToAdd.insert(vectorTileInfo);
                } else {
                    tilesToKeep.insert(vectorTileInfo);
                }
            }

            for (const auto &[tileInfo, _]: tiles) {
                bool found = false;
                for (const auto &currentTile: currentTileInfos) {
                    if (tileInfo == currentTile.tileInfo) {
                        found = true;
                        break;
                    }
                }
                if (!found) {
                    tilesToRemove.insert(tileInfo);
                }
            }
        }

        std::unordered_map<Tiled2dMapTileInfo, Tiled2dMapLayerMaskWrapper> newTileMasks;
        for (const auto &tileEntry : tilesToKeep) {

            size_t existingPolygonHash;
            {
                std::lock_guard<std::recursive_mutex> lock(tileMaskMapMutex);
                auto it = tileMaskMap.find(tileEntry.tileInfo);
                if (it != tileMaskMap.end()) {
                    existingPolygonHash = it->second.getPolygonHash();
                }
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

            {
                std::lock_guard<std::recursive_mutex> tilesReadyCountLock(tilesReadyCountMutex);
                tilesReadyCount[tile.tileInfo] = 0;
            }

            {
                std::lock_guard<std::recursive_mutex> lock(tilesMutex);
                tiles[tile.tileInfo] = {};
            }

            auto castedMe = std::static_pointer_cast<Tiled2dMapVectorLayer>(shared_from_this());
            auto selfActor = WeakActor<Tiled2dMapVectorLayer>(mailbox, castedMe);

            for (auto const &layer: mapDescription->layers) {
                if (!(layer->minZoom <= tile.tileInfo.zoomIdentifier && layer->maxZoom >= tile.tileInfo.zoomIdentifier)) {
                    continue;
                }
                auto const mapIt = tile.layerFeatureMaps.find(layer->source);
                if (mapIt == tile.layerFeatureMaps.end()) {
                    continue;
                }
                auto const dataIt = mapIt->second->find(layer->sourceId);
                if (dataIt != mapIt->second->end()) {
                    switch (layer->getType()) {
                        case VectorLayerType::background: {
                            break;
                        }
                        case VectorLayerType::line: {
                            break;
                        }
                        case VectorLayerType::polygon: {
                            auto mailbox = std::make_shared<Mailbox>(mapInterface->getScheduler());

                            auto actor = Actor<Tiled2dMapVectorPolygonTile>(mailbox, (std::weak_ptr<MapInterface>) mapInterface, tile.tileInfo, selfActor, *std::static_pointer_cast<PolygonVectorLayerDescription>(layer));

                            // TODO: deduplicate when other subtiles are created
                            {
                                tilesReadyCount[tile.tileInfo] += 1;
                                std::lock_guard<std::recursive_mutex> tilesReadyCountLock(tilesReadyCountMutex);
                            }

                            {
                                std::lock_guard<std::recursive_mutex> lock(tilesMutex);
                                tiles[tile.tileInfo].push_back(actor.strongActor<Tiled2dMapVectorTile>());
                            }

                            actor.message(&Tiled2dMapVectorTile::setTileData, nullptr, dataIt->second);
                            break;
                        }
                        case VectorLayerType::symbol: {
                            break;
                        }
                        case VectorLayerType::raster: {
                            break;
                        }
                        case VectorLayerType::custom: {
                            break;
                        }
                    }
                }
            }

            bool isAlreadyReady = false;
            {
                std::lock_guard<std::recursive_mutex> tilesReadyCountLock(tilesReadyCountMutex);
                isAlreadyReady = tilesReadyCount[tile.tileInfo] == 0;
            }
            if (isAlreadyReady) {
                vectorTileSource.message(&Tiled2dMapVectorSource::setTileReady, tile.tileInfo);
            }
        }

        if (!(newTileMasks.empty() && tilesToRemove.empty())) {
            auto castedMe = std::static_pointer_cast<Tiled2dMapVectorLayer>(shared_from_this());
            auto selfActor = WeakActor<Tiled2dMapVectorLayer>(mailbox, castedMe);
            selfActor.message(MailboxExecutionEnvironment::graphics, &Tiled2dMapVectorLayer::updateMaskObjects, newTileMasks, tilesToRemove);
        }
    }

    mapInterface->invalidate();
}

void Tiled2dMapVectorLayer::updateMaskObjects(const std::unordered_map<Tiled2dMapTileInfo, Tiled2dMapLayerMaskWrapper> toSetupMaskObject, const std::unordered_set<Tiled2dMapTileInfo> tilesToRemove) {
    auto mapInterface = this->mapInterface;
    auto renderingContext = mapInterface ? mapInterface->getRenderingContext() : nullptr;
    if (!renderingContext) return;

    for (const auto &[tileInfo, wrapper] : toSetupMaskObject) {

        wrapper.getGraphicsObject()->setup(renderingContext);

        std::shared_ptr<GraphicsObjectInterface> toClear;
        {
            std::lock_guard<std::recursive_mutex> lock(tileMaskMapMutex);
            auto it = tileMaskMap.find(tileInfo);
            if (it != tileMaskMap.end() && it->second.getGraphicsMaskObject()) {
                toClear = it->second.getGraphicsMaskObject()->asGraphicsObject();
            }
            tileMaskMap[tileInfo] = wrapper;
        }
        if (toClear) {
            toClear->clear();
        }

        {
            std::lock_guard<std::recursive_mutex> lock(tilesMutex);
            auto subTiles = tiles.find(tileInfo);
            if (subTiles != tiles.end()) {
                for (auto const &tile: subTiles->second) {
                    tile.syncAccess([maskObject = wrapper.getGraphicsMaskObject()](auto tile){
                        tile->updateTileMask(maskObject);
                    });
                }
            }
        }
    }

    for (const auto &tileToRemove: tilesToRemove) {
        {
            std::lock_guard<std::recursive_mutex> lock(tilesMutex);
            auto tilesVectorIt = tiles.find(tileToRemove);
            if (tilesVectorIt != tiles.end()) {
                for (const auto &tile: tiles.at(tileToRemove)) {
                    tile.syncAccess([](auto tile){
                        tile->clear();
                    });
                }
            }
            tiles.erase(tileToRemove);
        }

        {
            std::lock_guard<std::recursive_mutex> lock(tileMaskMapMutex);
            auto maskIt = tileMaskMap.find(tileToRemove);
            if (maskIt != tileMaskMap.end()) {
                const auto &object = maskIt->second.getGraphicsMaskObject()->asGraphicsObject();
                if (object->isReady()) object->clear();

                tileMaskMap.erase(tileToRemove);
            }
        }
        {
            std::lock_guard<std::recursive_mutex> tilesReadyLock(tilesReadyMutex);
            tilesReady.erase(tileToRemove);
        }
        {
            std::lock_guard<std::recursive_mutex> tilesReadyCountLock(tilesReadyCountMutex);
            tilesReadyCount.erase(tileToRemove);
        }
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
        vectorTileSource.message(&Tiled2dMapVectorSource::setTileReady, tile);
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
                                                                auto string = std::string((char*)dataResult.data->buf(), dataResult.data->len());
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
                                                                    // TODO: readd when SymbolTile available
                                                                    /*std::lock_guard<std::recursive_mutex> lock(selfPtr->tilesMutex);
                                                                    for (const auto &[tileInfo, subTiles]: selfPtr->tiles) {
                                                                        for (const auto &tile: subTiles) {
                                                                            if (auto symbolTile = std::dynamic_pointer_cast<Tiled2dMapVectorSymbolTile>(tile)) {
                                                                                symbolTile->setSprites(spriteTexture, spriteData);
                                                                            }
                                                                        }
                                                                    }*/
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
        std::lock_guard<std::recursive_mutex> lock(tilesMutex);
        for (const auto &[tileInfo, subTiles]: tiles) {
            for (const auto &tile: subTiles) {
                tile.message(&Tiled2dMapVectorTile::setScissorRect, scissorRect);
            }
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
        // TODO
        /*        std::lock_guard<std::recursive_mutex> lock(sublayerMutex);
        for (auto const &sublayer: sublayers) {
            auto vectorSubLayer = std::dynamic_pointer_cast<Tiled2dMapVectorSubLayer>(sublayer);
            if (vectorSubLayer) {
                vectorSubLayer->setSelectionDelegate(selectionDelegate);
            }
        }*/
    }
}

void Tiled2dMapVectorLayer::setSelectedFeatureIdentfier(std::optional<int64_t> identifier) {
    {
        // TODO
/*        std::lock_guard<std::recursive_mutex> lock(sublayerMutex);
        for (auto const &sublayer: sublayers) {
            auto vectorSubLayer = std::dynamic_pointer_cast<Tiled2dMapVectorSubLayer>(sublayer);
            if (vectorSubLayer) {
                vectorSubLayer->setSelectedFeatureIdentfier(identifier);
            }
        }*/
    }
}

void Tiled2dMapVectorLayer::updateLayerDescription(std::shared_ptr<VectorLayerDescription> layerDescription) {
    // TODO
/*
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
            newVectorSubLayer->onAdded(mapInterface, layerIndex);
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

        auto const &currentTileInfos = vectorTileSource.converse(&Tiled2dMapVectorSource::getCurrentTiles).get();

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
                                polygonObject = tileMaskMap[tile.tileInfo].getGraphicsMaskObject();
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
*/
}

std::optional<FeatureContext> Tiled2dMapVectorLayer::getFeatureContext(int64_t identifier) {
    auto const &currentTileInfos = vectorTileSource.converse(&Tiled2dMapVectorSource::getCurrentTiles).get();

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

// Touch Interface
bool Tiled2dMapVectorLayer::onTouchDown(const Vec2F &posScreen) {
    // TODO
/*    std::lock_guard<std::recursive_mutex> lock(sublayerMutex);
    for (auto rIter = sublayers.rbegin(); rIter != sublayers.rend(); rIter++) {
        const auto &touchInterface = std::dynamic_pointer_cast<TouchInterface>(*rIter);
        if (touchInterface) {
            if (touchInterface->onTouchDown(posScreen)) {
                return true;
            }
        }
    }
    return false;*/
    return false;
}

bool Tiled2dMapVectorLayer::onClickUnconfirmed(const Vec2F &posScreen) {
    // TODO
/*    std::lock_guard<std::recursive_mutex> lock(sublayerMutex);
    for (auto rIter = sublayers.rbegin(); rIter != sublayers.rend(); rIter++) {
        const auto &touchInterface = std::dynamic_pointer_cast<TouchInterface>(*rIter);
        if (touchInterface) {
            if (touchInterface->onClickUnconfirmed(posScreen)) {
                return true;
            }
        }
    }
    return false;*/
    return false;
}

bool Tiled2dMapVectorLayer::onClickConfirmed(const Vec2F &posScreen) {
    // TODO
/*    std::lock_guard<std::recursive_mutex> lock(sublayerMutex);
    for (auto rIter = sublayers.rbegin(); rIter != sublayers.rend(); rIter++) {
        const auto &touchInterface = std::dynamic_pointer_cast<TouchInterface>(*rIter);
        if (touchInterface) {
            if (touchInterface->onClickConfirmed(posScreen)) {
                return true;
            }
        }
    }
    return false;*/
    return false;
}

bool Tiled2dMapVectorLayer::onDoubleClick(const Vec2F &posScreen) {
    // TODO
/*    std::lock_guard<std::recursive_mutex> lock(sublayerMutex);
    for (auto rIter = sublayers.rbegin(); rIter != sublayers.rend(); rIter++) {
        const auto &touchInterface = std::dynamic_pointer_cast<TouchInterface>(*rIter);
        if (touchInterface) {
            if (touchInterface->onDoubleClick(posScreen)) {
                return true;
            }
        }
    }
    return false;*/
    return false;
}

bool Tiled2dMapVectorLayer::onLongPress(const Vec2F &posScreen) {
    // TODO
/*    std::lock_guard<std::recursive_mutex> lock(sublayerMutex);
    for (auto rIter = sublayers.rbegin(); rIter != sublayers.rend(); rIter++) {
        const auto &touchInterface = std::dynamic_pointer_cast<TouchInterface>(*rIter);
        if (touchInterface) {
            if (touchInterface->onLongPress(posScreen)) {
                return true;
            }
        }
    }
    return false;*/
    return false;
}

bool Tiled2dMapVectorLayer::onMove(const Vec2F &deltaScreen, bool confirmed, bool doubleClick) {
    // TODO
/*    std::lock_guard<std::recursive_mutex> lock(sublayerMutex);
    for (auto rIter = sublayers.rbegin(); rIter != sublayers.rend(); rIter++) {
        const auto &touchInterface = std::dynamic_pointer_cast<TouchInterface>(*rIter);
        if (touchInterface) {
            if (touchInterface->onMove(deltaScreen, confirmed, doubleClick)) {
                return true;
            }
        }
    }
    return false;*/
    return false;
}

bool Tiled2dMapVectorLayer::onMoveComplete() {
    // TODO
/*    std::lock_guard<std::recursive_mutex> lock(sublayerMutex);
    for (auto rIter = sublayers.rbegin(); rIter != sublayers.rend(); rIter++) {
        const auto &touchInterface = std::dynamic_pointer_cast<TouchInterface>(*rIter);
        if (touchInterface) {
            if (touchInterface->onMoveComplete()) {
                return true;
            }
        }
    }
    return false;*/
    return false;
}

bool Tiled2dMapVectorLayer::onTwoFingerClick(const Vec2F &posScreen1, const Vec2F &posScreen2) {
    // TODO
/*    std::lock_guard<std::recursive_mutex> lock(sublayerMutex);
    for (auto rIter = sublayers.rbegin(); rIter != sublayers.rend(); rIter++) {
        const auto &touchInterface = std::dynamic_pointer_cast<TouchInterface>(*rIter);
        if (touchInterface) {
            if (touchInterface->onTwoFingerClick(posScreen1, posScreen2)) {
                return true;
            }
        }
    }
    return false;*/
    return false;
}

bool Tiled2dMapVectorLayer::onTwoFingerMove(const std::vector<::Vec2F> &posScreenOld, const std::vector<::Vec2F> &posScreenNew) {
    // TODO
/*    std::lock_guard<std::recursive_mutex> lock(sublayerMutex);
    for (auto rIter = sublayers.rbegin(); rIter != sublayers.rend(); rIter++) {
        const auto &touchInterface = std::dynamic_pointer_cast<TouchInterface>(*rIter);
        if (touchInterface) {
            if (touchInterface->onTwoFingerMove(posScreenOld, posScreenNew)) {
                return true;
            }
        }
    }
    return false;*/
    return false;
}

bool Tiled2dMapVectorLayer::onTwoFingerMoveComplete() {
    // TODO
/*    std::lock_guard<std::recursive_mutex> lock(sublayerMutex);
    for (auto rIter = sublayers.rbegin(); rIter != sublayers.rend(); rIter++) {
        const auto &touchInterface = std::dynamic_pointer_cast<TouchInterface>(*rIter);
        if (touchInterface) {
            if (touchInterface->onTwoFingerMoveComplete()) {
                return true;
            }
        }
    }
    return false;*/
    return false;
}

void Tiled2dMapVectorLayer::clearTouch() {
    // TODO
/*    std::lock_guard<std::recursive_mutex> lock(sublayerMutex);
    for (auto rIter = sublayers.rbegin(); rIter != sublayers.rend(); rIter++) {
        const auto &touchInterface = std::dynamic_pointer_cast<TouchInterface>(*rIter);
        if (touchInterface) {
            touchInterface->clearTouch();
        }
    }*/
}

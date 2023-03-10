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
#include "Tiled2dMapVectorRasterSubLayerConfig.h"
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
#include "Tiled2dMapVectorLineTile.h"
#include "Tiled2dMapVectorRasterTile.h"


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
    
    auto selfMailbox = std::make_shared<Mailbox>(mapInterface->getScheduler());
    auto castedMe = std::static_pointer_cast<Tiled2dMapVectorLayer>(shared_from_this());
    auto selfRasterActor = WeakActor<Tiled2dMapRasterSourceListener>(selfMailbox, castedMe);
    auto selfVectorActor = WeakActor<Tiled2dMapVectorSourceListener>(selfMailbox, castedMe);

    std::vector<WeakActor<Tiled2dMapSourceInterface>> sourceInterfaces;
    std::vector<Actor<Tiled2dMapRasterSource>> rasterSources;
    std::unordered_map<std::string, std::unordered_set<std::string>> layersToDecode;

    for (auto const& layerDesc : mapDescription->layers)
    {
        switch (layerDesc->getType()) {
            case background: {
                // nothing
                break;
            }
            case raster: {
                auto sourceMailbox = std::make_shared<Mailbox>(mapInterface->getScheduler());
                auto rasterSource = std::make_shared<Tiled2dMapRasterSource>(mapInterface->getMapConfig(),
                                                                             std::make_shared<Tiled2dMapVectorRasterSubLayerConfig>(
                                                                                     std::static_pointer_cast<RasterVectorLayerDescription>(
                                                                                             layerDesc)),
                                                                             mapInterface->getCoordinateConverterHelper(),
                                                                             mapInterface->getScheduler(), loaders,
                                                                             selfRasterActor,
                                                                             mapInterface->getCamera()->getScreenDensityPpi());
                rasterSources.emplace_back(Actor<Tiled2dMapRasterSource>(mailbox, rasterSource));
                sourceInterfaces.emplace_back(WeakActor<Tiled2dMapSourceInterface>(mailbox, rasterSource));
                break;
            }
            case line:
            case polygon:
            case symbol:
            case custom: {
                layersToDecode[layerDesc->source].insert(layerDesc->sourceId);
                break;
            }
        }
    }
    this->rasterTileSources = rasterSources;

    auto sourceMailbox = std::make_shared<Mailbox>(mapInterface->getScheduler());
    vectorTileSource.emplaceObject(sourceMailbox, mapInterface->getMapConfig(),
                                   layerConfigs,
                                   mapInterface->getCoordinateConverterHelper(),
                                   mapInterface->getScheduler(),
                                   loaders,
                                   selfVectorActor,
                                   layersToDecode,
                                   mapInterface->getCamera()->getScreenDensityPpi());
    sourceInterfaces.push_back(vectorTileSource.weakActor<Tiled2dMapSourceInterface>());

    setSourceInterfaces(sourceInterfaces);

    Tiled2dMapLayer::onAdded(mapInterface, layerIndex);
    mapInterface->getTouchHandler()->insertListener(std::dynamic_pointer_cast<TouchInterface>(shared_from_this()), layerIndex);

    if (mapDescription->spriteBaseUrl) {
        loadSpriteData();
    }

    auto backgroundLayerDesc = std::find_if(mapDescription->layers.begin(), mapDescription->layers.end(), [](auto const &layer){
        return layer->getType() == VectorLayerType::background;
    });
    if (backgroundLayerDesc != mapDescription->layers.end()) {
        backgroundLayer = std::make_shared<Tiled2dMapVectorBackgroundSubLayer>(std::static_pointer_cast<BackgroundVectorLayerDescription>(*backgroundLayerDesc));
        backgroundLayer->onAdded(mapInterface, layerIndex);
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
}*/

    std::lock_guard<std::recursive_mutex> lock(tilesMutex);
    for (const auto &[tileInfo, subTiles] : tiles) {
        for (const auto &[index, identifier, tile]: subTiles) {
            tile.syncAccess([](auto tile) {
                tile->update();
            });
        }
    }
}

std::vector<std::shared_ptr<::RenderPassInterface>> Tiled2dMapVectorLayer::buildRenderPasses() {
    std::vector<std::shared_ptr<RenderPassInterface>> newPasses;

    if (backgroundLayer) {
        auto backgroundLayerPasses = backgroundLayer->buildRenderPasses();
        newPasses.insert(newPasses.end(), backgroundLayerPasses.begin(), backgroundLayerPasses.end());
    }

    std::lock_guard<std::recursive_mutex> lock(tilesMutex);
    for (const auto &[tileInfo, subTiles] : tiles) {
        for (const auto &[index, identifier, tile]: subTiles) {
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

    if (layerConfigs.empty()) {
        scheduleStyleJsonLoading();
        return;
    }

    // TODO: really necessary?
    //initializeVectorLayer();
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
            for (const auto &[index, identifier, tile]: subTiles) {
                tile.message(MailboxExecutionEnvironment::graphics, &Tiled2dMapVectorTile::clear);
            }
        }
        tiles.clear();
    }

    if (backgroundLayer) {
        backgroundLayer->onRemoved();
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
            for (const auto &[index, identifier, tile]: subTiles) {
                tile.message(MailboxExecutionEnvironment::graphics, &Tiled2dMapVectorTile::clear);
            }
        }
    }

    if (backgroundLayer) {
        backgroundLayer->pause();
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

    if (backgroundLayer) {
        backgroundLayer->resume();
    }

    {
        std::lock_guard<std::recursive_mutex> lock(tilesMutex);
        for (const auto &[tileInfo, subTiles] : tiles) {
            for (const auto &[index, identifier, tile]: subTiles) {
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
            for (const auto &[index, identifier, tile]: subTiles) {
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

void Tiled2dMapVectorLayer::onTilesUpdated(const std::string &layerName, std::unordered_set<Tiled2dMapRasterTileInfo> currentTileInfos) {
    auto lockSelfPtr = std::static_pointer_cast<Tiled2dMapVectorLayer>(shared_from_this());
    auto mapInterface = lockSelfPtr ? lockSelfPtr->mapInterface : nullptr;
    {
        auto graphicsFactory = mapInterface ? mapInterface->getGraphicsObjectFactory() : nullptr;
        auto coordinateConverterHelper = mapInterface ? mapInterface->getCoordinateConverterHelper() : nullptr;
        auto shaderFactory = mapInterface ? mapInterface->getShaderFactory() : nullptr;
        if (!graphicsFactory || !shaderFactory) {
            return;
        }

        std::unordered_set<Tiled2dMapRasterTileInfo> tilesToAdd;
        std::unordered_set<Tiled2dMapRasterTileInfo> tilesToKeep;

        std::unordered_set<Tiled2dMapTileInfo> tilesToRemove;

        {
            std::lock_guard<std::recursive_mutex> lock(tilesMutex);

            for (const auto &rasterTileInfo: currentTileInfos) {
                if (tiles.count(rasterTileInfo.tileInfo) == 0) {
                    tilesToAdd.insert(rasterTileInfo);
                    //LogDebug <<= "UBCM: Raster to add " + std::to_string(rasterTileInfo.tileInfo.x) + "/" + std::to_string(rasterTileInfo.tileInfo.y);
                } else {
                    tilesToKeep.insert(rasterTileInfo);
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
                    //LogDebug <<= "UBCM: Raster to remove " + std::to_string(tileInfo.x) + "/" + std::to_string(tileInfo.y);
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
                tilesReadyCount[tile.tileInfo] = 0; // TODO: UBCM - set to number of subtiles
            }

            {
                std::lock_guard<std::recursive_mutex> lock(tilesMutex);
                tiles[tile.tileInfo] = {};
            }

            for (int32_t index = 0; index < mapDescription->layers.size(); index++) {
                auto const &layer= mapDescription->layers.at(index);
                if (layer->getType() != VectorLayerType::raster || layer->identifier != layerName) {
                    continue;
                }

                if (!(layer->minZoom <= tile.tileInfo.zoomIdentifier && layer->maxZoom >= tile.tileInfo.zoomIdentifier)) {
                    continue;
                }
                const auto data = tile.textureHolder;
                if (data) {

                    std::string identifier = layer->identifier;
                    Actor<Tiled2dMapVectorTile> actor = createTileActor(tile.tileInfo, layer);

                    if (actor) {
                        if (selectionDelegate) {
                            actor.message(&Tiled2dMapVectorTile::setSelectionDelegate, selectionDelegate);
                        }

                        {
                            std::lock_guard<std::recursive_mutex> tilesReadyCountLock(tilesReadyCountMutex);
                            tilesReadyCount[tile.tileInfo] += 1; // TODO: UBCM remove
                        }


                        {
                            std::lock_guard<std::recursive_mutex> lock(tilesMutex);
                            tiles[tile.tileInfo].push_back({index, identifier, actor.strongActor<Tiled2dMapVectorTile>()});
                        }

                        actor.message(&Tiled2dMapVectorTile::setTileData, nullptr, data);
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

void Tiled2dMapVectorLayer::onTilesUpdated(const std::string &sourceName, std::unordered_set<Tiled2dMapVectorTileInfo> currentTileInfos) {
    return;

    auto lockSelfPtr = std::static_pointer_cast<Tiled2dMapVectorLayer>(shared_from_this());
    auto mapInterface = lockSelfPtr ? lockSelfPtr->mapInterface : nullptr;
    {
        auto graphicsFactory = mapInterface ? mapInterface->getGraphicsObjectFactory() : nullptr;
        auto coordinateConverterHelper = mapInterface ? mapInterface->getCoordinateConverterHelper() : nullptr;
        auto shaderFactory = mapInterface ? mapInterface->getShaderFactory() : nullptr;
        if (!graphicsFactory || !shaderFactory) {
            return;
        }

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
                    //LogDebug <<= "UBCM: Vec to remove " + std::to_string(tileInfo.x) + "/" + std::to_string(tileInfo.y);
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
                tilesReadyCount[tile.tileInfo] = 0; // TODO: UBCM - set to number of subtiles
            }

            {
                std::lock_guard<std::recursive_mutex> lock(tilesMutex);
                tiles[tile.tileInfo] = {};
            }

            for (int32_t index = 0; index < mapDescription->layers.size(); index++) {
                auto const &layer= mapDescription->layers.at(index);
                if (!(layer->minZoom <= tile.tileInfo.zoomIdentifier && layer->maxZoom >= tile.tileInfo.zoomIdentifier)) {
                    continue;
                }
                auto const mapIt = tile.layerFeatureMaps.find(layer->source);
                if (mapIt == tile.layerFeatureMaps.end()) {
                    continue;
                }
                auto const dataIt = mapIt->second->find(layer->sourceId);
                if (dataIt != mapIt->second->end()) {

                    std::string identifier = layer->identifier;
                    Actor<Tiled2dMapVectorTile> actor = createTileActor(tile.tileInfo, layer);

                    if (actor) {
                        if (selectionDelegate) {
                            actor.message(&Tiled2dMapVectorTile::setSelectionDelegate, selectionDelegate);
                        }

                        {
                            std::lock_guard<std::recursive_mutex> tilesReadyCountLock(tilesReadyCountMutex);
                            tilesReadyCount[tile.tileInfo] += 1; // TODO: UBCM remove
                        }

                        {
                            std::lock_guard<std::recursive_mutex> lock(tilesMutex);
                            tiles[tile.tileInfo].push_back({index, identifier, actor.strongActor<Tiled2dMapVectorTile>()});
                        }

                        actor.message(&Tiled2dMapVectorTile::setTileData, nullptr, dataIt->second);
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

Actor<Tiled2dMapVectorTile> Tiled2dMapVectorLayer::createTileActor(const Tiled2dMapTileInfo &tileInfo,
                                                                   const std::shared_ptr<VectorLayerDescription> &layerDescription) {
    Actor<Tiled2dMapVectorTile> actor;

    auto castedMe = std::static_pointer_cast<Tiled2dMapVectorLayer>(shared_from_this());
    auto selfActor = WeakActor<Tiled2dMapVectorLayer>(mailbox, castedMe);

    switch (layerDescription->getType()) {
        case VectorLayerType::background: {
            break;
        }
        case VectorLayerType::line: {
/*            auto mailbox = std::make_shared<Mailbox>(mapInterface->getScheduler());

            auto lineActor = Actor<Tiled2dMapVectorLineTile>(mailbox, (std::weak_ptr<MapInterface>) mapInterface, tileInfo,
                                                             selfActor, std::static_pointer_cast<LineVectorLayerDescription>(
                            layerDescription));

            actor = lineActor.strongActor<Tiled2dMapVectorTile>();
            break;*/
        }
        case VectorLayerType::polygon: {
/*            auto mailbox = std::make_shared<Mailbox>(mapInterface->getScheduler());

            auto polygonActor = Actor<Tiled2dMapVectorPolygonTile>(mailbox, (std::weak_ptr<MapInterface>) mapInterface,
                                                                   tileInfo, selfActor,
                                                                   std::static_pointer_cast<PolygonVectorLayerDescription>(
                                                                           layerDescription));

            actor = polygonActor.strongActor<Tiled2dMapVectorTile>();
            break;*/
        }
        case VectorLayerType::symbol: {
            break;
        }
        case VectorLayerType::raster: {
            auto mailbox = std::make_shared<Mailbox>(mapInterface->getScheduler());

            auto rasterActor = Actor<Tiled2dMapVectorRasterTile>(mailbox, (std::weak_ptr<MapInterface>) mapInterface,
                                                                   tileInfo, selfActor,
                                                                   std::static_pointer_cast<RasterVectorLayerDescription>(
                                                                           layerDescription));

            actor = rasterActor.strongActor<Tiled2dMapVectorTile>();
            break;
        }
        case VectorLayerType::custom: {
            break;
        }
    }
    return std::move(actor);
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
                for (auto const &[index, identifier, tile]: subTiles->second) {
                    tile.syncAccess([tileI = tileInfo, maskObject = wrapper.getGraphicsMaskObject()](auto tile){
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
                for (const auto &[index, identifier, tile]: tiles.at(tileToRemove)) {
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
    mapInterface->invalidate();
}


void Tiled2dMapVectorLayer::tileIsReady(const Tiled2dMapTileInfo &tile) {
    {
        std::lock_guard<std::recursive_mutex> tilesReadyLock(tilesReadyMutex);
        if (tilesReady.count(tile) > 0) return;
    }
    bool isCompletelyReady = false;
    {
        std::lock_guard<std::recursive_mutex> tilesReadyCountLock(tilesReadyCountMutex);
        if (tilesReadyCount.count(tile) == 0) {
            return;
        }
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
        for (const auto &rasterSource : rasterTileSources) {
            rasterSource.message(&Tiled2dMapRasterSource::setTileReady, tile);
        }
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
            for (const auto &[index, identifier, tile]: subTiles) {
                tile.message(&Tiled2dMapVectorTile::setScissorRect, scissorRect);
            }
        }
    }
    auto mapInterface = this->mapInterface;
    if (mapInterface) {
        mapInterface->invalidate();
    }
}

void Tiled2dMapVectorLayer::setSelectionDelegate(const WeakActor<Tiled2dMapVectorLayerSelectionInterface> &selectionDelegate) {
    this->selectionDelegate = selectionDelegate;
    {
        std::lock_guard<std::recursive_mutex> lock(tilesMutex);
        for (const auto &[tileInfo, subTiles]: tiles) {
            for (const auto &[index, identifier, tile]: subTiles) {
                tile.message(&Tiled2dMapVectorTile::setSelectionDelegate, selectionDelegate);
            }
        }
    }
}

void Tiled2dMapVectorLayer::setSelectedFeatureIdentifier(std::optional<int64_t> identifier) {
    {
       std::lock_guard<std::recursive_mutex> lock(tilesMutex);
        for (auto const &[tileInfo, subTiles]: tiles) {
            for (const auto &[index, tileIdentifier, tile]: subTiles) {
                tile.message(MailboxDuplicationStrategy::replaceNewest, &Tiled2dMapVectorTile::setSelectedFeatureIdentifier, identifier);
            }
        }
    }
    auto mapInterface = this->mapInterface;
    if (mapInterface) {
        mapInterface->invalidate();
    }
}

void Tiled2dMapVectorLayer::updateLayerDescription(std::shared_ptr<VectorLayerDescription> layerDescription) {
    auto mapInterface = this->mapInterface;
    if (!mapInterface) {
        return;
    }

    std::shared_ptr<VectorLayerDescription> legacyDescription;
    int32_t legacyIndex = -1;
    size_t numLayers = mapDescription->layers.size();
    for (int index = 0; index < numLayers; index++) {
        if (mapDescription->layers[index]->identifier == layerDescription->identifier) {
            legacyDescription = mapDescription->layers[index];
            legacyIndex = index;
            mapDescription->layers[index] = layerDescription;
            break;
        }
    }

    if (legacyIndex < 0) {
        return;
    }

    // Evaluate if a complete replacement of the tiles is needed (source/zoom adjustments may lead to a different set of created tiles)
    bool needsTileReplace = legacyDescription->source != layerDescription->source
                            || legacyDescription->sourceId != layerDescription->sourceId
                            || legacyDescription->minZoom != layerDescription->minZoom
                            || legacyDescription->maxZoom != layerDescription->maxZoom;

    auto const &currentTileInfos = vectorTileSource.converse(&Tiled2dMapVectorSource::getCurrentTiles).get();

    {
        auto castedMe = std::static_pointer_cast<Tiled2dMapVectorLayer>(shared_from_this());
        auto selfActor = WeakActor<Tiled2dMapVectorLayer>(mailbox, castedMe);

        for (const auto &tileData: currentTileInfos) {

            std::shared_ptr<MaskingObjectInterface> tileMask;
            {
                std::lock_guard<std::recursive_mutex> lock(tileMaskMapMutex);
                auto tileMaskIter = tileMaskMap.find(tileData.tileInfo);
                if (tileMaskIter != tileMaskMap.end()) {
                    tileMask = tileMaskIter->second.getGraphicsMaskObject();
                }
            }

            std::lock_guard<std::recursive_mutex> lock(tilesMutex);
            auto subTiles = tiles.find(tileData.tileInfo);
            if (subTiles == tiles.end()) {
                continue;
            }

            if (needsTileReplace) {
                // Remove invalid legacy tile (only one - identifier is unique)
                subTiles->second.erase(std::remove_if(subTiles->second.begin(), subTiles->second.end(),
                                                      [&identifier = layerDescription->identifier]
                                                              (const std::tuple<int32_t, std::string, Actor<Tiled2dMapVectorTile>> &subTile) {
                                                          return std::get<1>(subTile) == identifier;
                                                      }));

                // Re-evaluate criteria for the tile creation of this specific sublayer
                if (!(layerDescription->minZoom <= tileData.tileInfo.zoomIdentifier && layerDescription->maxZoom >= tileData.tileInfo.zoomIdentifier)) {
                    continue;
                }
                auto const mapIt = tileData.layerFeatureMaps.find(layerDescription->source);
                if (mapIt == tileData.layerFeatureMaps.end()) {
                    continue;
                }
                auto const dataIt = mapIt->second->find(layerDescription->sourceId);
                if (dataIt == mapIt->second->end()) {
                    continue;
                }

                Actor<Tiled2dMapVectorTile> actor = createTileActor(tileData.tileInfo, layerDescription);
                if (actor) {
                    if (selectionDelegate) {
                        actor.message(&Tiled2dMapVectorTile::setSelectionDelegate, selectionDelegate);
                    }

                    if (subTiles->second.empty()) {
                        subTiles->second.push_back({legacyIndex, layerDescription->identifier, actor.strongActor<Tiled2dMapVectorTile>()});
                    } else {
                        for (auto subTileIter = subTiles->second.begin(); subTileIter != subTiles->second.end(); subTileIter++) {
                            if (std::get<0>(*subTileIter) > legacyIndex) {
                                subTiles->second.insert(subTileIter - 1, {legacyIndex, layerDescription->identifier, actor.strongActor<Tiled2dMapVectorTile>()});
                                break;
                            }
                        }
                    }

                    actor.message(&Tiled2dMapVectorTile::setTileData, tileMask, dataIt->second);
                }

            } else {
                for (auto &[index, identifier, tile]  : subTiles->second) {
                    if (identifier == layerDescription->identifier) {
                        auto const mapIt = tileData.layerFeatureMaps.find(layerDescription->source);
                        if (mapIt == tileData.layerFeatureMaps.end()) {
                            break;
                        }
                        auto const dataIt = mapIt->second->find(layerDescription->sourceId);
                        if (dataIt == mapIt->second->end()) {
                            break;
                        }
                        //tile.message(&Tiled2dMapVectorTile::updateLayerDescription, layerDescription, dataIt->second);
                        break;
                    }
                }
            }

        }
    }
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
                return layer->clone();
            }
        }
    }
    return nullptr;
}

// Touch Interface
bool Tiled2dMapVectorLayer::onTouchDown(const Vec2F &posScreen) {
    // TODO: query ordering on tile borders?
    std::lock_guard<std::recursive_mutex> lock(tilesMutex);
    for (const auto &[tileInfo, subTiles]: tiles) {
        for (auto rIter = subTiles.rbegin(); rIter != subTiles.rend(); rIter++) {
            bool hit = std::get<2>(*rIter).syncAccess([posScreen](auto tile) {
                return tile->onTouchDown(posScreen);
            });
            if (hit) {
                return true;
            }
        }
    }
    return false;
}

bool Tiled2dMapVectorLayer::onClickUnconfirmed(const Vec2F &posScreen) {
// TODO: query ordering on tile borders?
    std::lock_guard<std::recursive_mutex> lock(tilesMutex);
    for (const auto &[tileInfo, subTiles]: tiles) {
        for (auto rIter = subTiles.rbegin(); rIter != subTiles.rend(); rIter++) {
            bool hit = std::get<2>(*rIter).syncAccess([posScreen](auto tile) {
                return tile->onClickUnconfirmed(posScreen);
            });
            if (hit) {
                return true;
            }
        }
    }
    return false;
}

bool Tiled2dMapVectorLayer::onClickConfirmed(const Vec2F &posScreen) {
// TODO: query ordering on tile borders?
    std::lock_guard<std::recursive_mutex> lock(tilesMutex);
    for (const auto &[tileInfo, subTiles]: tiles) {
        for (auto rIter = subTiles.rbegin(); rIter != subTiles.rend(); rIter++) {
            bool hit = std::get<2>(*rIter).syncAccess([posScreen](auto tile) {
                return tile->onClickConfirmed(posScreen);
            });
            if (hit) {
                return true;
            }
        }
    }
    return false;
}

bool Tiled2dMapVectorLayer::onDoubleClick(const Vec2F &posScreen) {
// TODO: query ordering on tile borders?
    std::lock_guard<std::recursive_mutex> lock(tilesMutex);
    for (const auto &[tileInfo, subTiles]: tiles) {
        for (auto rIter = subTiles.rbegin(); rIter != subTiles.rend(); rIter++) {
            bool hit = std::get<2>(*rIter).syncAccess([posScreen](auto tile) {
                return tile->onDoubleClick(posScreen);
            });
            if (hit) {
                return true;
            }
        }
    }
    return false;
}

bool Tiled2dMapVectorLayer::onLongPress(const Vec2F &posScreen) {
// TODO: query ordering on tile borders?
    std::lock_guard<std::recursive_mutex> lock(tilesMutex);
    for (const auto &[tileInfo, subTiles]: tiles) {
        for (auto rIter = subTiles.rbegin(); rIter != subTiles.rend(); rIter++) {
            bool hit = std::get<2>(*rIter).syncAccess([posScreen](auto tile) {
                return tile->onLongPress(posScreen);
            });
            if (hit) {
                return true;
            }
        }
    }
    return false;
}

bool Tiled2dMapVectorLayer::onMove(const Vec2F &deltaScreen, bool confirmed, bool doubleClick) {
// TODO: query ordering on tile borders?
    std::lock_guard<std::recursive_mutex> lock(tilesMutex);
    for (const auto &[tileInfo, subTiles]: tiles) {
        for (auto rIter = subTiles.rbegin(); rIter != subTiles.rend(); rIter++) {
            bool hit = std::get<2>(*rIter).syncAccess([deltaScreen, confirmed, doubleClick](auto tile) {
                return tile->onMove(deltaScreen, confirmed, doubleClick);
            });
            if (hit) {
                return true;
            }
        }
    }
    return false;
}

bool Tiled2dMapVectorLayer::onMoveComplete() {
// TODO: query ordering on tile borders?
    std::lock_guard<std::recursive_mutex> lock(tilesMutex);
    for (const auto &[tileInfo, subTiles]: tiles) {
        for (auto rIter = subTiles.rbegin(); rIter != subTiles.rend(); rIter++) {
            bool hit = std::get<2>(*rIter).syncAccess([](auto tile) {
                return tile->onMoveComplete();
            });
            if (hit) {
                return true;
            }
        }
    }
    return false;
}

bool Tiled2dMapVectorLayer::onTwoFingerClick(const Vec2F &posScreen1, const Vec2F &posScreen2) {
// TODO: query ordering on tile borders?
    std::lock_guard<std::recursive_mutex> lock(tilesMutex);
    for (const auto &[tileInfo, subTiles]: tiles) {
        for (auto rIter = subTiles.rbegin(); rIter != subTiles.rend(); rIter++) {
            bool hit = std::get<2>(*rIter).syncAccess([posScreen1, posScreen2](auto tile) {
                return tile->onTwoFingerClick(posScreen1, posScreen2);
            });
            if (hit) {
                return true;
            }
        }
    }
    return false;
}

bool Tiled2dMapVectorLayer::onTwoFingerMove(const std::vector<::Vec2F> &posScreenOld, const std::vector<::Vec2F> &posScreenNew) {
// TODO: query ordering on tile borders?
    std::lock_guard<std::recursive_mutex> lock(tilesMutex);
    for (const auto &[tileInfo, subTiles]: tiles) {
        for (auto rIter = subTiles.rbegin(); rIter != subTiles.rend(); rIter++) {
            bool hit = std::get<2>(*rIter).syncAccess([posScreenOld, posScreenNew](auto tile) {
                return tile->onTwoFingerMove(posScreenOld, posScreenNew);
            });
            if (hit) {
                return true;
            }
        }
    }
    return false;
}

bool Tiled2dMapVectorLayer::onTwoFingerMoveComplete() {
// TODO: query ordering on tile borders?
    std::lock_guard<std::recursive_mutex> lock(tilesMutex);
    for (const auto &[tileInfo, subTiles]: tiles) {
        for (auto rIter = subTiles.rbegin(); rIter != subTiles.rend(); rIter++) {
            bool hit = std::get<2>(*rIter).syncAccess([](auto tile) {
                return tile->onTwoFingerMoveComplete();
            });
            if (hit) {
                return true;
            }
        }
    }
    return false;
}

void Tiled2dMapVectorLayer::clearTouch() {
// TODO: query ordering on tile borders?
    std::lock_guard<std::recursive_mutex> lock(tilesMutex);
    for (const auto &[tileInfo, subTiles]: tiles) {
        for (auto rIter = subTiles.rbegin(); rIter != subTiles.rend(); rIter++) {
            std::get<2>(*rIter).syncAccess([](auto tile) {
                return tile->clearTouch();
            });
        }
    }
}

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
        dpFactor(dpFactor) {}


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

        dpFactor(dpFactor) {
        }

Tiled2dMapVectorLayer::Tiled2dMapVectorLayer(const std::string &layerName, const std::shared_ptr<VectorMapDescription> &mapDescription,
                                             const std::vector<std::shared_ptr<::LoaderInterface>> &loaders,
                                             const std::shared_ptr<::FontLoaderInterface> &fontLoader) :
        Tiled2dMapLayer(),
        layerName(layerName),
        loaders(loaders),
        fontLoader(fontLoader) {
    setMapDescription(mapDescription);
}

Tiled2dMapVectorLayer::Tiled2dMapVectorLayer(const std::string &layerName,
                                             const std::vector<std::shared_ptr<::LoaderInterface>> &loaders,
                                             const std::shared_ptr<::FontLoaderInterface> &fontLoader) :
        Tiled2dMapLayer(),
        layerName(layerName),
        loaders(loaders),
        fontLoader(fontLoader) {
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

    std::lock_guard<std::recursive_mutex> lock(dataManagerMutex);
    for (const auto &[source, sourceDataManager]: sourceDataManagers) {
        sourceDataManager.syncAccess([](auto manager) {
            manager->update();
        });
    }
}

std::vector<std::shared_ptr<::RenderPassInterface>> Tiled2dMapVectorLayer::buildRenderPasses() {
    std::vector<std::shared_ptr<RenderPassInterface>> newPasses;

    if (backgroundLayer) {
        auto backgroundLayerPasses = backgroundLayer->buildRenderPasses();
        newPasses.insert(newPasses.end(), backgroundLayerPasses.begin(), backgroundLayerPasses.end());
    }

    std::vector<std::tuple<int32_t, std::vector<std::shared_ptr<RenderPassInterface>>>> orderedPasses;
    std::lock_guard<std::recursive_mutex> lock(dataManagerMutex);
    for (const auto &[source, sourceDataManager] : sourceDataManagers) {
        auto passes = sourceDataManager.syncAccess([](auto manager) {
            return manager->buildRenderPasses();
        });
        orderedPasses.insert(orderedPasses.end(), passes.begin(), passes.end());
    }

    std::sort(orderedPasses.begin(), orderedPasses.end(), [](const auto &lhs, const auto &rhs) {
        return std::get<0>(lhs) < std::get<0>(rhs);
    });

    for (const auto &[index, passes] : orderedPasses) {
        newPasses.insert(newPasses.end(), passes.begin(), passes.end());
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
    for (const auto &rasterSource : rasterTileSources) {
        rasterSource.message(&Tiled2dMapRasterSource::pause);
    }

    {
        std::lock_guard<std::recursive_mutex> lock(dataManagerMutex);
        for (const auto &[source, sourceDataManager]: sourceDataManagers) {
            sourceDataManager.message(MailboxExecutionEnvironment::graphics, &Tiled2dMapVectorSourceTileDataManager::pause);
        }
    }

    if (backgroundLayer) {
        backgroundLayer->pause();
    }
}

void Tiled2dMapVectorLayer::resume() {
    isResumed = true;

    if (vectorTileSource) {
        vectorTileSource.message(&Tiled2dMapVectorSource::resume);
    }
    for (const auto &rasterSource : rasterTileSources) {
        rasterSource.message(&Tiled2dMapRasterSource::resume);
    }

    if (backgroundLayer) {
        backgroundLayer->resume();
    }

    {
        std::lock_guard<std::recursive_mutex> lock(dataManagerMutex);
        for (const auto &[source, sourceDataManager]: sourceDataManagers) {
            sourceDataManager.message(MailboxExecutionEnvironment::graphics, &Tiled2dMapVectorSourceTileDataManager::resume);
        }
    }

}

void Tiled2dMapVectorLayer::setAlpha(float alpha) {
    if (this->alpha == alpha) {
        return;
    }
    this->alpha = alpha;
    {
        std::lock_guard<std::recursive_mutex> lock(dataManagerMutex);
        for (const auto &[source, sourceDataManager]: sourceDataManagers) {
            sourceDataManager.message(&Tiled2dMapVectorSourceTileDataManager::setAlpha, alpha);
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
    std::lock_guard<std::recursive_mutex> lock(dataManagerMutex);
    for (const auto &[source, sourceDataManager]: sourceDataManagers) {
        sourceDataManager.message(&Tiled2dMapVectorSourceTileDataManager::onRasterTilesUpdated, layerName, currentTileInfos);
    }
}

void Tiled2dMapVectorLayer::onTilesUpdated(const std::string &sourceName, std::unordered_set<Tiled2dMapVectorTileInfo> currentTileInfos) {
    std::lock_guard<std::recursive_mutex> lock(dataManagerMutex);
    for (const auto &[source, sourceDataManager]: sourceDataManagers) {
        sourceDataManager.message(&Tiled2dMapVectorSourceTileDataManager::onVectorTilesUpdated, sourceName, currentTileInfos);
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
    this->scissorRect = scissorRect;
    auto mapInterface = this->mapInterface;
    if (mapInterface) {
        mapInterface->invalidate();
    }
}

void Tiled2dMapVectorLayer::setSelectionDelegate(const WeakActor<Tiled2dMapVectorLayerSelectionInterface> &selectionDelegate) {
    std::lock_guard<std::recursive_mutex> lock(dataManagerMutex);
    for (const auto &[source, sourceDataManager]: sourceDataManagers) {
        sourceDataManager.message(&Tiled2dMapVectorSourceTileDataManager::setSelectionDelegate, selectionDelegate);
    }
}

void Tiled2dMapVectorLayer::setSelectedFeatureIdentifier(std::optional<int64_t> identifier) {
    {
        std::lock_guard<std::recursive_mutex> lock(dataManagerMutex);
        for (const auto &[source, sourceDataManager]: sourceDataManagers) {
            sourceDataManager.message(MailboxDuplicationStrategy::replaceNewest, &Tiled2dMapVectorSourceTileDataManager::setSelectedFeatureIdentifier, identifier);
        }
    }
    auto mapInterface = this->mapInterface;
    if (mapInterface) {
        mapInterface->invalidate();
    }
}

void Tiled2dMapVectorLayer::updateLayerDescription(std::shared_ptr<VectorLayerDescription> layerDescription) {
    std::lock_guard<std::recursive_mutex> lock(dataManagerMutex);
    for (const auto &[source, sourceDataManager]: sourceDataManagers) {
        sourceDataManager.message(&Tiled2dMapVectorSourceTileDataManager::updateLayerDescription, layerDescription);
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
/*    // TODO: query ordering on tile borders?
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
    }*/
    return false;
}

bool Tiled2dMapVectorLayer::onClickUnconfirmed(const Vec2F &posScreen) {
/*// TODO: query ordering on tile borders?
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
    }*/
    return false;
}

bool Tiled2dMapVectorLayer::onClickConfirmed(const Vec2F &posScreen) {
/*// TODO: query ordering on tile borders?
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
    }*/
    return false;
}

bool Tiled2dMapVectorLayer::onDoubleClick(const Vec2F &posScreen) {
/*// TODO: query ordering on tile borders?
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
    }*/
    return false;
}

bool Tiled2dMapVectorLayer::onLongPress(const Vec2F &posScreen) {
/*// TODO: query ordering on tile borders?
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
    }*/
    return false;
}

bool Tiled2dMapVectorLayer::onMove(const Vec2F &deltaScreen, bool confirmed, bool doubleClick) {
/*// TODO: query ordering on tile borders?
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
    }*/
    return false;
}

bool Tiled2dMapVectorLayer::onMoveComplete() {
/*// TODO: query ordering on tile borders?
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
    }*/
    return false;
}

bool Tiled2dMapVectorLayer::onTwoFingerClick(const Vec2F &posScreen1, const Vec2F &posScreen2) {
/*// TODO: query ordering on tile borders?
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
    }*/
    return false;
}

bool Tiled2dMapVectorLayer::onTwoFingerMove(const std::vector<::Vec2F> &posScreenOld, const std::vector<::Vec2F> &posScreenNew) {
/*// TODO: query ordering on tile borders?
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
    }*/
    return false;
}

bool Tiled2dMapVectorLayer::onTwoFingerMoveComplete() {
/*// TODO: query ordering on tile borders?
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
    }*/
    return false;
}

void Tiled2dMapVectorLayer::clearTouch() {
/*// TODO: query ordering on tile borders?
    std::lock_guard<std::recursive_mutex> lock(tilesMutex);
    for (const auto &[tileInfo, subTiles]: tiles) {
        for (auto rIter = subTiles.rbegin(); rIter != subTiles.rend(); rIter++) {
            std::get<2>(*rIter).syncAccess([](auto tile) {
                return tile->clearTouch();
            });
        }
    }*/
}

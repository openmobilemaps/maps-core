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
#include "Tiled2dMapVectorLayerParserHelper.h"
#include "RasterVectorLayerDescription.h"
#include "LineVectorLayerDescription.h"
#include "PolygonVectorLayerDescription.h"
#include "SymbolVectorLayerDescription.h"
#include "BackgroundVectorLayerDescription.h"
#include "VectorTileGeometryHandler.h"
#include "Tiled2dMapVectorBackgroundSubLayer.h"
#include "Tiled2dMapVectorRasterSubLayerConfig.h"
#include "Polygon2dInterface.h"
#include "MapCamera2dInterface.h"
#include "QuadMaskObject.h"
#include "PolygonMaskObject.h"
#include "CoordinatesUtil.h"
#include "RenderPass.h"
#include "DataLoaderResult.h"
#include "TextureLoaderResult.h"
#include "PolygonCompare.h"
#include "LoaderHelper.h"
#include "Tiled2dMapVectorPolygonTile.h"
#include "Tiled2dMapVectorLineTile.h"
#include "Tiled2dMapVectorRasterTile.h"
#include "SpriteData.h"
#include "DateHelper.h"
#include "Tiled2dMapVectorBackgroundSubLayer.h"
#include "Tiled2dMapVectorSourceTileDataManager.h"
#include "Tiled2dMapVectorSourceRasterTileDataManager.h"
#include "Tiled2dMapVectorSourceVectorTileDataManager.h"
#include "Tiled2dMapVectorSourceSymbolDataManager.h"
#include "Tiled2dMapVectorSourceSymbolCollisionManager.h"
#include "Tiled2dMapVectorInteractionManager.h"
#include "Tiled2dMapVectorReadyManager.h"
#include "Tiled2dVectorGeoJsonSource.h"
#include "Tiled2dMapVectorStyleParser.h"
#include "Tiled2dMapVectorGeoJSONLayerConfig.h"
#include "GeoJsonVTFactory.h"

Tiled2dMapVectorLayer::Tiled2dMapVectorLayer(const std::string &layerName,
                                             const std::optional<std::string> &remoteStyleJsonUrl,
                                             const std::vector<std::shared_ptr<::LoaderInterface>> &loaders,
                                             const std::shared_ptr<::FontLoaderInterface> &fontLoader,
                                             const std::optional<Tiled2dMapZoomInfo> &customZoomInfo,
                                             const std::shared_ptr<Tiled2dMapVectorLayerSymbolDelegateInterface> &symbolDelegate,
                                             const std::unordered_map<std::string, std::string> & sourceUrlParams,
                                             const std::shared_ptr<Tiled2dMapVectorLayerLocalDataProviderInterface> &localDataProvider) :
        Tiled2dMapLayer(),
        layerName(layerName),
        remoteStyleJsonUrl(remoteStyleJsonUrl),
        loaders(loaders),
        fontLoader(fontLoader),
        customZoomInfo(customZoomInfo),
        featureStateManager(std::make_shared<Tiled2dMapVectorStateManager>()),
        symbolDelegate(symbolDelegate),
        sourceUrlParams(sourceUrlParams),
        localDataProvider(localDataProvider)
        {}

Tiled2dMapVectorLayer::Tiled2dMapVectorLayer(const std::string &layerName,
                                             const std::optional<std::string> &remoteStyleJsonUrl,
                                             const std::optional<std::string> &fallbackStyleJsonString,
                                             const std::vector<std::shared_ptr<::LoaderInterface>> &loaders,
                                             const std::shared_ptr<::FontLoaderInterface> &fontLoader,
                                             const std::optional<Tiled2dMapZoomInfo> &customZoomInfo,
                                             const std::shared_ptr<Tiled2dMapVectorLayerSymbolDelegateInterface> &symbolDelegate,
                                             const std::unordered_map<std::string, std::string> & sourceUrlParams,
                                             const std::shared_ptr<Tiled2dMapVectorLayerLocalDataProviderInterface> &localDataProvider) :
        Tiled2dMapLayer(),
        layerName(layerName),
        remoteStyleJsonUrl(remoteStyleJsonUrl),
        fallbackStyleJsonString(fallbackStyleJsonString),
        loaders(loaders),
        fontLoader(fontLoader),
        customZoomInfo(customZoomInfo),
        featureStateManager(std::make_shared<Tiled2dMapVectorStateManager>()),
        symbolDelegate(symbolDelegate),
        sourceUrlParams(sourceUrlParams),
        localDataProvider(localDataProvider)
        {}


Tiled2dMapVectorLayer::Tiled2dMapVectorLayer(const std::string &layerName,
                                             const std::shared_ptr<VectorMapDescription> &mapDescription,
                                             const std::vector<std::shared_ptr<::LoaderInterface>> &loaders,
                                             const std::shared_ptr<::FontLoaderInterface> &fontLoader,
                                             const std::optional<Tiled2dMapZoomInfo> &customZoomInfo,
                                             const std::shared_ptr<Tiled2dMapVectorLayerSymbolDelegateInterface> &symbolDelegate,
                                             const std::shared_ptr<Tiled2dMapVectorLayerLocalDataProviderInterface> &localDataProvider,
                                             const std::unordered_map<std::string, std::string> & sourceUrlParams
                                             ) :
        Tiled2dMapLayer(),
        layerName(layerName),
        loaders(loaders),
        fontLoader(fontLoader),
        customZoomInfo(customZoomInfo),
        featureStateManager(std::make_shared<Tiled2dMapVectorStateManager>()),
        symbolDelegate(symbolDelegate),
        localDataProvider(localDataProvider),
		sourceUrlParams(sourceUrlParams) {
    	setMapDescription(mapDescription);
}

Tiled2dMapVectorLayer::Tiled2dMapVectorLayer(const std::string &layerName,
                                             const std::vector<std::shared_ptr<::LoaderInterface>> &loaders,
                                             const std::shared_ptr<::FontLoaderInterface> &fontLoader,
                                             const std::optional<Tiled2dMapZoomInfo> &customZoomInfo,
                                             const std::shared_ptr<Tiled2dMapVectorLayerSymbolDelegateInterface> &symbolDelegate,
                                             const std::unordered_map<std::string, std::string> & sourceUrlParams
                                             ) :
        Tiled2dMapLayer(),
        layerName(layerName),
        loaders(loaders),
        fontLoader(fontLoader),
        customZoomInfo(customZoomInfo),
        featureStateManager(std::make_shared<Tiled2dMapVectorStateManager>()),
        symbolDelegate(symbolDelegate),
        sourceUrlParams(sourceUrlParams)
        {}

void Tiled2dMapVectorLayer::scheduleStyleJsonLoading() {
    isLoadingStyleJson = true;
    auto mapInterface = this->mapInterface;
    auto scheduler = mapInterface ? mapInterface->getScheduler() : nullptr;
    if (!scheduler) {
        return;
    }
    std::weak_ptr<Tiled2dMapVectorLayer> weakSelfPtr = std::dynamic_pointer_cast<Tiled2dMapVectorLayer>(shared_from_this());
    scheduler->addTask(std::make_shared<LambdaTask>(
            TaskConfig("VectorTile_loadStyleJson", 0, TaskPriority::NORMAL, ExecutionEnvironment::IO),
            [weakSelfPtr] {
                auto selfPtr = weakSelfPtr.lock();
                if (selfPtr) {
                    auto layerError = selfPtr->loadStyleJson();
                    if (selfPtr->errorManager) {
                        if (auto error = layerError) {
                            selfPtr->errorManager->addTiledLayerError(*error);
                        } else {
                            if (selfPtr->remoteStyleJsonUrl.has_value() && !selfPtr->remoteStyleJsonUrl->empty()) {
                                selfPtr->errorManager->removeError(*selfPtr->remoteStyleJsonUrl);
                            } else {
                                selfPtr->errorManager->removeError(selfPtr->layerName);
                            }
                        }
                    }

                    selfPtr->isLoadingStyleJson = false;
                    selfPtr->didLoadStyleJson(layerError);
                }
            }));
}

void Tiled2dMapVectorLayer::didLoadStyleJson(const std::optional<TiledLayerError> &error) {

}

std::optional<TiledLayerError> Tiled2dMapVectorLayer::loadStyleJson() {
    auto error = loadStyleJsonRemotely();
    if (error.has_value() || !this->remoteStyleJsonUrl.has_value()) {
        if (localDataProvider) {
            auto optionalStyleJson = localDataProvider->getStyleJson();
            if (optionalStyleJson) {
                auto parseResult = Tiled2dMapVectorLayerParserHelper::parseStyleJsonFromString(layerName, *optionalStyleJson, localDataProvider, loaders, sourceUrlParams);
                
                if (parseResult.status == LoaderStatus::OK) {
                    setMapDescription(parseResult.mapDescription);
                    metadata = parseResult.metadata;
                    return std::nullopt;
                }
            }
        }
        
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
    if (!remoteStyleJsonUrl.has_value()) {
        return std::nullopt;
    }
    auto parseResult = Tiled2dMapVectorLayerParserHelper::parseStyleJsonFromUrl(layerName, *remoteStyleJsonUrl, localDataProvider, loaders, sourceUrlParams);
    if (parseResult.status == LoaderStatus::OK) {
        setMapDescription(parseResult.mapDescription);
        metadata = parseResult.metadata;
        return std::nullopt;
    } else {
        return TiledLayerError(parseResult.status, parseResult.errorCode, layerName,
                                               *remoteStyleJsonUrl, parseResult.status != LoaderStatus::ERROR_404 ||
                                                               parseResult.status != LoaderStatus::ERROR_400, std::nullopt);
    }
}

std::optional<TiledLayerError> Tiled2dMapVectorLayer::loadStyleJsonLocally(std::string styleJsonString) {
    auto parseResult = Tiled2dMapVectorLayerParserHelper::parseStyleJsonFromString(layerName, styleJsonString, localDataProvider, loaders, sourceUrlParams);

    if (parseResult.status == LoaderStatus::OK) {
        setMapDescription(parseResult.mapDescription);
        metadata = parseResult.metadata;
        return std::nullopt;
    } else {
        return TiledLayerError(parseResult.status, parseResult.errorCode, layerName,
                               layerName, false, std::nullopt);
    }
}

std::shared_ptr<Tiled2dMapVectorLayerConfig>
Tiled2dMapVectorLayer::getLayerConfig(const std::shared_ptr<VectorMapSourceDescription> &source) {
    return customZoomInfo.has_value() ? std::make_shared<Tiled2dMapVectorLayerConfig>(source, *customZoomInfo)
                                      : std::make_shared<Tiled2dMapVectorLayerConfig>(source);
}

std::shared_ptr<Tiled2dMapVectorLayerConfig>
Tiled2dMapVectorLayer::getGeoJSONLayerConfig(const std::string &sourceName, const std::shared_ptr<GeoJSONVTInterface> &source) {
    return customZoomInfo.has_value() ? std::make_shared<Tiled2dMapVectorGeoJSONLayerConfig>(sourceName, source, *customZoomInfo)
                                      : std::make_shared<Tiled2dMapVectorGeoJSONLayerConfig>(sourceName, source);
}

void Tiled2dMapVectorLayer::setMapDescription(const std::shared_ptr<VectorMapDescription> &mapDescription) {
    {
        std::lock_guard<std::recursive_mutex> lock(mapDescriptionMutex);
        this->mapDescription = mapDescription;
        this->persistingSymbolPlacement = mapDescription->persistingSymbolPlacement;
        this->layerConfigs.clear();

        for (auto const &source: mapDescription->vectorSources) {
            layerConfigs[source->identifier] = getLayerConfig(source);
        }
        for (auto const &[source, geoJson]: mapDescription->geoJsonSources) {
            layerConfigs[source] = getGeoJSONLayerConfig(source, geoJson);
        }
    }

    initializeVectorLayer();
    applyGlobalOrFeatureStateIfPossible(StateType::BOTH);
}

void Tiled2dMapVectorLayer::initializeVectorLayer() {

    std::lock_guard<std::recursive_mutex> lock(mapDescriptionMutex);

    if (!sourceDataManagers.empty() || !symbolSourceDataManagers.empty() || !rasterTileSources.empty()) {
        // do nothing if the layer is already initialized

        Tiled2dMapLayer::onAdded(mapInterface, layerIndex);
        mapInterface->getTouchHandler()->insertListener(std::dynamic_pointer_cast<TouchInterface>(shared_from_this()), layerIndex);

        if (backgroundLayer) {
            backgroundLayer->onAdded(mapInterface, layerIndex);
        }

        if (!isResumed) {
            resume();
        }
        return;
    }

    auto mapInterface = this->mapInterface;
    if (!mapInterface) {
        return;
    }
    
    std::shared_ptr<Mailbox> selfMailbox = mailbox;
    if (!mailbox) {
        selfMailbox = std::make_shared<Mailbox>(mapInterface->getScheduler());
    }
    auto castedMe = std::static_pointer_cast<Tiled2dMapVectorLayer>(shared_from_this());
    auto selfActor = WeakActor<Tiled2dMapVectorLayer>(selfMailbox, castedMe);
    auto selfRasterActor = WeakActor<Tiled2dMapRasterSourceListener>(selfMailbox, castedMe);
    auto selfVectorActor = WeakActor<Tiled2dMapVectorSourceListener>(selfMailbox, castedMe);

    std::vector<WeakActor<Tiled2dMapSourceInterface>> sourceInterfaces;
    std::vector<Actor<Tiled2dMapRasterSource>> rasterSources;

    std::unordered_map<std::string, Actor<Tiled2dMapVectorSource>> vectorTileSources;

    std::unordered_map<std::string, Actor<Tiled2dMapVectorSourceTileDataManager>> sourceTileManagers;
    std::unordered_map<std::string, Actor<Tiled2dMapVectorSourceSymbolDataManager>> symbolSourceDataManagers;
    std::unordered_map<std::string, std::vector<WeakActor<Tiled2dMapVectorSourceDataManager>>> interactionDataManagers;

    std::unordered_map<std::string, std::unordered_set<std::string>> layersToDecode;

    std::unordered_set<std::string> symbolSources;

    for (auto const& layerDesc : mapDescription->layers)
    {
        switch (layerDesc->getType()) {
            case background: {
                // nothing
                break;
            }
            case raster: {
                auto rasterSubLayerConfig = customZoomInfo.has_value() ? std::make_shared<Tiled2dMapVectorRasterSubLayerConfig>(
                        std::static_pointer_cast<RasterVectorLayerDescription>(layerDesc), *customZoomInfo)
                                                                       : std::make_shared<Tiled2dMapVectorRasterSubLayerConfig>(
                                std::static_pointer_cast<RasterVectorLayerDescription>(layerDesc));

                auto sourceMailbox = std::make_shared<Mailbox>(mapInterface->getScheduler());
                auto sourceActor = Actor<Tiled2dMapRasterSource>(sourceMailbox,
                                                                 mapInterface->getMapConfig(),
                                                                 rasterSubLayerConfig,
                                                                 mapInterface->getCoordinateConverterHelper(),
                                                                 mapInterface->getScheduler(), loaders,
                                                                 selfRasterActor,
                                                                 mapInterface->getCamera()->getScreenDensityPpi(),
                                                                 layerName);
                rasterSources.push_back(sourceActor);


                auto readyManagerMailbox = std::make_shared<Mailbox>(mapInterface->getScheduler());
                auto readyManager = Actor<Tiled2dMapVectorReadyManager>(readyManagerMailbox, sourceActor.weakActor<Tiled2dMapSourceReadyInterface>());

                auto sourceDataManagerMailbox = std::make_shared<Mailbox>(mapInterface->getScheduler());
                auto sourceManagerActor = Actor<Tiled2dMapVectorSourceRasterTileDataManager>(sourceDataManagerMailbox,
                                                                                             selfActor,
                                                                                             mapDescription,
                                                                                             rasterSubLayerConfig,
                                                                                             layerDesc,
                                                                                             sourceActor.weakActor<Tiled2dMapRasterSource>(),
                                                                                             readyManager,
                                                                                             featureStateManager,
                                                                                             mapInterface->getCamera()->getScreenDensityPpi() / 160.0);
                sourceManagerActor.unsafe()->setAlpha(alpha);
                sourceTileManagers[layerDesc->source] = sourceManagerActor.strongActor<Tiled2dMapVectorSourceTileDataManager>();
                sourceInterfaces.push_back(sourceActor.weakActor<Tiled2dMapSourceInterface>());
                interactionDataManagers[layerDesc->source].push_back(sourceManagerActor.weakActor<Tiled2dMapVectorSourceDataManager>());
                break;
            }
            case symbol: {
                symbolSources.insert(layerDesc->source);
            }
            case line:
            case polygon:
            case custom: {
                layersToDecode[layerDesc->source].insert(layerDesc->sourceLayer);
                break;
            }
        }
    }

    for (auto const &[source, layers]: layersToDecode) {


        auto layerConfig = layerConfigs[source];

        if (!layerConfig) {
            LogError << "Missing layer config for " << source <<= ", layer will be ignored.";
            continue;
        }

        auto sourceMailbox = std::make_shared<Mailbox>(mapInterface->getScheduler());

        Actor<Tiled2dMapVectorSource> vectorSource;
        auto geoJsonSourceIt = mapDescription->geoJsonSources.find(source);
        if (geoJsonSourceIt != mapDescription->geoJsonSources.end()) {
            auto geoJsonSource = Actor<Tiled2dVectorGeoJsonSource>(sourceMailbox,
                                                                   mapInterface->getCamera(),
                                                                   mapInterface->getMapConfig(),
                                                                   layerConfig,
                                                                   mapInterface->getCoordinateConverterHelper(),
                                                                   mapInterface->getScheduler(),
                                                                   loaders,
                                                                   selfVectorActor,
                                                                   layers,
                                                                   source,
                                                                   mapInterface->getCamera()->getScreenDensityPpi(),
                                                                   mapDescription->geoJsonSources.at(source),
                                                                   layerName);
            vectorSource = geoJsonSource.strongActor<Tiled2dMapVectorSource>();

            geoJsonSourceIt->second->setDelegate(geoJsonSource.weakActor<GeoJSONTileDelegate>());

        } else {
            vectorSource = Actor<Tiled2dMapVectorSource>(sourceMailbox,
                                                              mapInterface->getMapConfig(),
                                                              layerConfig,
                                                              mapInterface->getCoordinateConverterHelper(),
                                                              mapInterface->getScheduler(),
                                                              loaders,
                                                              selfVectorActor,
                                                              layers,
                                                              source,
                                                              mapInterface->getCamera()->getScreenDensityPpi(),
                                                              layerName);
        }
        vectorTileSources[source] = vectorSource;
        sourceInterfaces.push_back(vectorSource.weakActor<Tiled2dMapSourceInterface>());

        auto readyManagerMailbox = std::make_shared<Mailbox>(mapInterface->getScheduler());
        auto readyManager = Actor<Tiled2dMapVectorReadyManager>(readyManagerMailbox, vectorSource.weakActor<Tiled2dMapSourceReadyInterface>());

        auto sourceDataManagerMailbox = std::make_shared<Mailbox>(mapInterface->getScheduler());
        auto sourceManagerActor = Actor<Tiled2dMapVectorSourceVectorTileDataManager>(sourceDataManagerMailbox,
                                                                                     selfActor,
                                                                                     mapDescription,
                                                                                     layerConfig,
                                                                                     source,
                                                                                     vectorSource.weakActor<Tiled2dMapVectorSource>(),
                                                                                     readyManager,
                                                                                     featureStateManager);
        sourceManagerActor.unsafe()->setAlpha(alpha);
        sourceTileManagers[source] = sourceManagerActor.strongActor<Tiled2dMapVectorSourceTileDataManager>();
        interactionDataManagers[source].push_back(sourceManagerActor.weakActor<Tiled2dMapVectorSourceDataManager>());

        if (symbolSources.count(source) != 0) {
            auto symbolSourceDataManagerMailbox = std::make_shared<Mailbox>(mapInterface->getScheduler());
            auto actor = Actor<Tiled2dMapVectorSourceSymbolDataManager>(symbolSourceDataManagerMailbox,
                                                                        selfActor,
                                                                        mapDescription,
                                                                        layerConfig,
                                                                        source,
                                                                        fontLoader,
                                                                        vectorSource.weakActor<Tiled2dMapVectorSource>(),
                                                                        readyManager,
                                                                        featureStateManager,
                                                                        symbolDelegate,
                                                                        mapDescription->persistingSymbolPlacement);
            actor.unsafe()->setAlpha(alpha);
            actor.unsafe()->enableAnimations(animationsEnabled);
            symbolSourceDataManagers[source] = actor;
            interactionDataManagers[source].push_back(actor.weakActor<Tiled2dMapVectorSourceDataManager>());
        }
    }

    std::unordered_map<std::string, WeakActor<Tiled2dMapVectorSourceSymbolDataManager>> weakSymbolSourceDataManagers;
    for (const auto &sourceTileManager : symbolSourceDataManagers) {
        weakSymbolSourceDataManagers[sourceTileManager.first] = sourceTileManager.second.weakActor<Tiled2dMapVectorSourceSymbolDataManager>();
    }

    if(!weakSymbolSourceDataManagers.empty()) {
        auto collisionManagerMailbox = std::make_shared<Mailbox>(mapInterface->getScheduler());
        collisionManager.emplaceObject(collisionManagerMailbox, weakSymbolSourceDataManagers, mapDescription);
    }

    interactionManager = std::make_unique<Tiled2dMapVectorInteractionManager>(interactionDataManagers, mapDescription);

    this->rasterTileSources = rasterSources;
    this->vectorTileSources = vectorTileSources;
    this->symbolSourceDataManagers = symbolSourceDataManagers;
    this->sourceDataManagers = sourceTileManagers;

    if(strongSelectionDelegate) {
        setSelectionDelegate(strongSelectionDelegate);
    } else if (auto ptr = selectionDelegate.lock()) {
        setSelectionDelegate(selectionDelegate);
    }

    setSourceInterfaces(sourceInterfaces);

    Tiled2dMapLayer::onAdded(mapInterface, layerIndex);
    mapInterface->getTouchHandler()->insertListener(std::dynamic_pointer_cast<TouchInterface>(shared_from_this()), layerIndex);

    for (const auto &sourceTileManager : sourceTileManagers) {
        sourceTileManager.second.message(&Tiled2dMapVectorSourceTileDataManager::onAdded, mapInterface);
    }
    for (const auto &sourceTileManager : symbolSourceDataManagers) {
        sourceTileManager.second.message(&Tiled2dMapVectorSourceTileDataManager::onAdded, mapInterface);
    }

    auto scale = mapInterface->getCamera()->getScreenDensityPpi() > 326.0 ? 3 : (mapInterface->getCamera()->getScreenDensityPpi() >= 264.0 ? 2 : 1);

    if (mapDescription->spriteBaseUrl) {
        loadSpriteData(scale);
    }

    auto backgroundLayerDesc = std::find_if(mapDescription->layers.begin(), mapDescription->layers.end(), [](auto const &layer){
        return layer->getType() == VectorLayerType::background;
    });
    if (backgroundLayerDesc != mapDescription->layers.end()) {
        backgroundLayer = std::make_shared<Tiled2dMapVectorBackgroundSubLayer>(std::static_pointer_cast<BackgroundVectorLayerDescription>(*backgroundLayerDesc), featureStateManager);
        if (spriteData && spriteTexture) {
            backgroundLayer->setSprites(spriteData, spriteTexture);
        }
        backgroundLayer->onAdded(mapInterface, layerIndex);
    }

    if (isResumed) {
        for (const auto &[source, vectorTileSource] : vectorTileSources) {
            vectorTileSource.message(&Tiled2dMapVectorSource::resume);
        }

        for (const auto &rasterSource : rasterSources) {
            rasterSource.message(&Tiled2dMapRasterSource::resume);
        }
    }

    {
        std::unique_lock<std::mutex> lock(setupMutex);
        setupReady = true;
    }
    setupCV.notify_all();
}

void Tiled2dMapVectorLayer::reloadDataSource(const std::string &sourceName) {

    Actor<Tiled2dMapVectorSource> source;
    std::shared_ptr<GeoJSONVTInterface> geoSource = nullptr;

    {
        std::lock_guard<std::recursive_mutex> lock(mapDescriptionMutex);
        source = vectorTileSources[sourceName];
        geoSource = mapDescription->geoJsonSources[sourceName];
    }

    if (source && geoSource) {
        source.syncAccess([&,geoSource](const auto &source) {

            geoSource->reload(loaders);
            auto promise = std::make_shared<::djinni::Promise<std::shared_ptr<DataLoaderResult>>>();
            geoSource->waitIfNotLoaded(promise);
            promise->getFuture().wait();

            source->reloadTiles();
        });
    }

}

void Tiled2dMapVectorLayer::reloadLocalDataSource(const std::string &sourceName, const std::string &geoJson) {
    std::lock_guard<std::recursive_mutex> lock(mapDescriptionMutex);

    auto mapInterface = this->mapInterface;
    auto mapDescription = this->mapDescription;

    if (!mapInterface || !mapDescription) {
        return;
    }

    if (const auto &geoSource = mapDescription->geoJsonSources[sourceName]) {

        nlohmann::json json;

        try {
            json = nlohmann::json::parse(geoJson);
        }
        catch (nlohmann::json::parse_error &ex) {
            return;
        }

        geoSource->reload(GeoJsonParser::getGeoJson(json));
    }
    if (auto &source = vectorTileSources[sourceName]) {
        source.syncAccess([](const auto &source) {
            source->reloadTiles();
        });
    }

    prevCollisionStillValid.clear();
    tilesStillValid.clear();
    mapInterface->invalidate();
}

std::shared_ptr<::LayerInterface> Tiled2dMapVectorLayer::asLayerInterface() {
    return shared_from_this();
}

void Tiled2dMapVectorLayer::update() {
    if (isHidden) {
        return;
    }
    long long now = DateHelper::currentTimeMillis();
    for (const auto &[source, sourceDataManager]: sourceDataManagers) {
        sourceDataManager.syncAccess([](const auto &manager) {
            manager->update();
        });
    }

    if (collisionManager) {
        auto mapInterface = this->mapInterface;
        auto renderingContext = mapInterface ? mapInterface->getRenderingContext() : nullptr;
        auto camera = mapInterface ? mapInterface->getCamera() : nullptr;
        if (!camera) {
            return;
        }
        double newZoom = camera->getZoom();
        auto now = DateHelper::currentTimeMillis();
        bool newIsAnimating = false;
        bool tilesChanged = !tilesStillValid.test_and_set();
        double zoomChange = abs(newZoom-lastDataManagerZoom) / std::max(newZoom, 1.0);
        double timeDiff = now - lastDataManagerUpdate;
        if (zoomChange > 0.001 || timeDiff > 1000 || isAnimating || tilesChanged) {
            lastDataManagerUpdate = now;
            lastDataManagerZoom = newZoom;

            Vec2I viewportSize = renderingContext->getViewportSize();
            float viewportRotation = camera->getRotation();
            std::optional<std::vector<float>> vpMatrix = camera->getLastVpMatrix();
            if (!vpMatrix) return;
            for (const auto &[source, sourceDataManager]: symbolSourceDataManagers) {
                bool a = sourceDataManager.syncAccess([&now](const auto &manager) {
                    return manager->update(now);
                });
                newIsAnimating |= a;
            }
            isAnimating = newIsAnimating;
            if (now - lastCollitionCheck > 1000 || tilesChanged || zoomChange > 0.001) {
                lastCollitionCheck = now;
                bool enforceUpdate = !prevCollisionStillValid.test_and_set();
                collisionManager.syncAccess(
                        [&vpMatrix, &viewportSize, viewportRotation, enforceUpdate, persistingPlacement = this->persistingSymbolPlacement](
                                const auto &manager) {
                            manager->collisionDetection(*vpMatrix, viewportSize, viewportRotation, enforceUpdate,
                                                        persistingPlacement);
                        });
                isAnimating = true;
            }
        }


    }
}

std::vector<std::shared_ptr<::RenderPassInterface>> Tiled2dMapVectorLayer::buildRenderPasses() {
    if (isHidden) {
        return {};
    }
    std::lock_guard<std::recursive_mutex> lock(renderPassMutex);
    return currentRenderPasses;
}

void Tiled2dMapVectorLayer::onRenderPassUpdate(const std::string &source, bool isSymbol, const std::vector<std::shared_ptr<TileRenderDescription>> &renderDescription) {
    if (isSymbol) {
        sourceRenderDescriptionMap[source].symbolRenderDescriptions = renderDescription;
    } else {
        sourceRenderDescriptionMap[source].renderDescriptions = renderDescription;
    }
    pregenerateRenderPasses();
    updateReadyStateListenerIfNeeded();
    prevCollisionStillValid.clear();
}

void Tiled2dMapVectorLayer::pregenerateRenderPasses() {
    std::vector<std::shared_ptr<RenderPassInterface>> newPasses;

    if (backgroundLayer) {
        auto backgroundLayerPasses = backgroundLayer->buildRenderPasses();
        newPasses.insert(newPasses.end(), backgroundLayerPasses.begin(), backgroundLayerPasses.end());
    }

    std::vector<std::shared_ptr<TileRenderDescription>> orderedRenderDescriptions;
    for (const auto &[source, indexPasses] : sourceRenderDescriptionMap) {
        orderedRenderDescriptions.insert(orderedRenderDescriptions.end(), indexPasses.renderDescriptions.begin(), indexPasses.renderDescriptions.end());
        orderedRenderDescriptions.insert(orderedRenderDescriptions.end(), indexPasses.symbolRenderDescriptions.begin(), indexPasses.symbolRenderDescriptions.end());
    }

    std::sort(orderedRenderDescriptions.begin(), orderedRenderDescriptions.end(), [](const auto &lhs, const auto &rhs) {
        return lhs->layerIndex < rhs->layerIndex;
    });

    std::vector<std::shared_ptr<::RenderObjectInterface>> renderObjects;
    std::shared_ptr<MaskingObjectInterface> lastMask = nullptr;
    int32_t lastRenderPassIndex = 0;

    for (const auto &description : orderedRenderDescriptions) {
        if (description->renderObjects.empty()) {
            continue;
        }
        if ((description->renderPassIndex != lastRenderPassIndex || description->maskingObject != lastMask) && !renderObjects.empty()) {
            newPasses.emplace_back(std::make_shared<RenderPass>(RenderPassConfig(lastRenderPassIndex, false), renderObjects, lastMask));
            renderObjects.clear();
            lastMask = nullptr;
            lastRenderPassIndex = 0;
        }

        if (description->isModifyingMask || description->selfMasked) {
            if (!renderObjects.empty()) {
                newPasses.emplace_back(std::make_shared<RenderPass>(RenderPassConfig(description->renderPassIndex, false), renderObjects, lastMask));
            }
            renderObjects.clear();
            lastMask = nullptr;
            newPasses.emplace_back(std::make_shared<RenderPass>(RenderPassConfig(description->renderPassIndex, description->selfMasked), description->renderObjects, description->maskingObject));
        } else {
            renderObjects.insert(renderObjects.end(), description->renderObjects.begin(), description->renderObjects.end());
            lastMask = description->maskingObject;
            lastRenderPassIndex = description->renderPassIndex;
        }
    }
    if (!renderObjects.empty()) {
        newPasses.emplace_back(std::make_shared<RenderPass>(RenderPassConfig(lastRenderPassIndex, false), renderObjects, lastMask));
        renderObjects.clear();
        lastMask = nullptr;
    }

    if (scissorRect) {
        for(const auto &pass: newPasses) {
            std::static_pointer_cast<RenderPass>(pass)->setScissoringRect(scissorRect);
        }
    }

    {
        std::lock_guard<std::recursive_mutex> lock(renderPassMutex);
        currentRenderPasses = newPasses;
    }
}

void Tiled2dMapVectorLayer::onAdded(const std::shared_ptr<::MapInterface> &mapInterface, int32_t layerIndex) {
    this->mapInterface = mapInterface;
    this->layerIndex = layerIndex;

    if (layerConfigs.empty()) {
        scheduleStyleJsonLoading();
        return;
    }

    // this is needed if the layer is initialized with a style.json string
    initializeVectorLayer();
}

void Tiled2dMapVectorLayer::onRemoved() {
    auto const mapInterface = this->mapInterface;
    if (mapInterface) {
        mapInterface->getTouchHandler()->removeListener(std::dynamic_pointer_cast<TouchInterface>(shared_from_this()));
    }
    Tiled2dMapLayer::onRemoved();

    if (backgroundLayer) {
        backgroundLayer->onRemoved();
    }

    this->layerIndex = -1;
}

void Tiled2dMapVectorLayer::pause() {
    isResumed = false;

    for (const auto &[source, sourceDataManager]: sourceDataManagers) {
        sourceDataManager.syncAccess([](const auto &manager){
            manager->pause();
        });
    }
    for (const auto &[source, sourceDataManager]: symbolSourceDataManagers) {
        sourceDataManager.syncAccess([](const auto &manager){
            manager->pause();
        });
    }

    if (backgroundLayer) {
        backgroundLayer->pause();
    }
}

void Tiled2dMapVectorLayer::resume() {
    isResumed = true;

    if (backgroundLayer) {
        backgroundLayer->resume();
    }

    for (const auto &[source, sourceDataManager]: sourceDataManagers) {
        sourceDataManager.syncAccess([](const auto &manager){
            manager->resume();
        });
    }
    for (const auto &[source, sourceDataManager]: symbolSourceDataManagers) {
        sourceDataManager.syncAccess([](const auto &manager){
            manager->resume();
        });
    }

    for (const auto &source: sourceInterfaces) {
        source.message(&Tiled2dMapSourceInterface::notifyTilesUpdates);
    }
}

void Tiled2dMapVectorLayer::setAlpha(float alpha) {
    if (this->alpha == alpha) {
        return;
    }
    this->alpha = alpha;
    for (const auto &[source, sourceDataManager]: sourceDataManagers) {
        sourceDataManager.message(&Tiled2dMapVectorSourceTileDataManager::setAlpha, alpha);
    }
    for (const auto &[source, sourceDataManager]: symbolSourceDataManagers) {
        sourceDataManager.message(&Tiled2dMapVectorSourceSymbolDataManager::setAlpha, alpha);
    }

    if (mapInterface)
        mapInterface->invalidate();
}

float Tiled2dMapVectorLayer::getAlpha() { return alpha; }


void Tiled2dMapVectorLayer::forceReload() {
    std::lock_guard<std::recursive_mutex> lock(mapDescriptionMutex);
    if (!isLoadingStyleJson && (remoteStyleJsonUrl.has_value() || localDataProvider) && !mapDescription && vectorTileSources.empty()) {
        scheduleStyleJsonLoading();
        return;
    }
    Tiled2dMapLayer::forceReload();
}

void Tiled2dMapVectorLayer::onTilesUpdated(const std::string &layerName, std::unordered_set<Tiled2dMapRasterTileInfo> currentTileInfos) {

    std::unique_lock<std::mutex> lock(setupMutex);
    setupCV.wait(lock, [this]{ return setupReady; });

    auto sourceManager = sourceDataManagers.find(layerName);
    if (sourceManager != sourceDataManagers.end()) {
        sourceManager->second.message(MailboxDuplicationStrategy::replaceNewest, &Tiled2dMapVectorSourceTileDataManager::onRasterTilesUpdated, layerName, currentTileInfos);
    }
    tilesStillValid.clear();
}

void Tiled2dMapVectorLayer::onTilesUpdated(const std::string &sourceName, std::unordered_set<Tiled2dMapVectorTileInfo> currentTileInfos) {

    std::unique_lock<std::mutex> lock(setupMutex);
    setupCV.wait(lock, [this]{ return setupReady; });

    auto sourceManager = sourceDataManagers.find(sourceName);
    if (sourceManager != sourceDataManagers.end()) {
        sourceManager->second.message(MailboxDuplicationStrategy::replaceNewest, &Tiled2dMapVectorSourceTileDataManager::onVectorTilesUpdated, sourceName, currentTileInfos);
    }
    auto symbolSourceManager = symbolSourceDataManagers.find(sourceName);
    if (symbolSourceManager != symbolSourceDataManagers.end()) {
        symbolSourceManager->second.message(MailboxDuplicationStrategy::replaceNewest, &Tiled2dMapVectorSourceTileDataManager::onVectorTilesUpdated, sourceName, currentTileInfos);
    }
    tilesStillValid.clear();
}

void Tiled2dMapVectorLayer::loadSpriteData(int scale, bool fromLocal) {
    auto lockSelfPtr = shared_from_this();
    auto mapInterface = this->mapInterface;
    auto camera = mapInterface ? mapInterface->getCamera() : nullptr;
    auto scheduler = mapInterface ? mapInterface->getScheduler() : nullptr;
    if (!camera || !scheduler) {
        return;
    }

    std::string scalePrefix = (scale == 3 ? "@3x" : (scale == 2 ? "@2x" : ""));
    std::stringstream ssTexture;
    std::stringstream ssData;
    {
        std::lock_guard<std::recursive_mutex> lock(mapDescriptionMutex);
        ssTexture << *mapDescription->spriteBaseUrl << scalePrefix << ".png";
        ssData << *mapDescription->spriteBaseUrl << scalePrefix << ".json";
    }
    std::string urlTexture = ssTexture.str();
    std::string urlData = ssData.str();

    struct Context {
        std::atomic<size_t> counter;

        std::shared_ptr<DataLoaderResult> jsonResult;
        std::shared_ptr<TextureLoaderResult> textureResult;

        djinni::Promise<void> promise;
        Context(size_t c) : counter(c) {}
    };

    auto context = std::make_shared<Context>(2);

    std::shared_ptr<::djinni::Future<::DataLoaderResult>> jsonLoaderFuture;
    if(localDataProvider && fromLocal) {
        jsonLoaderFuture = std::make_shared<::djinni::Future<::DataLoaderResult>>(localDataProvider->loadSpriteJsonAsync(scale));
    } else {
        jsonLoaderFuture = std::make_shared<::djinni::Future<::DataLoaderResult>>(LoaderHelper::loadDataAsync(urlData, std::nullopt, loaders));
    }

    auto castedMe = std::static_pointer_cast<Tiled2dMapVectorLayer>(shared_from_this());
    std::weak_ptr<Tiled2dMapVectorLayer> weakSelf = castedMe;
    jsonLoaderFuture->then([context, scale, weakSelf, fromLocal] (auto result) {
        context->jsonResult =  std::make_shared<DataLoaderResult>(result.get());
        
        if (--(context->counter) == 0) {
            context->promise.setValue();
        }
    });

    std::shared_ptr<::djinni::Future<::TextureLoaderResult>> textureLoaderFuture;
    if(localDataProvider) {
        textureLoaderFuture = std::make_shared<::djinni::Future<::TextureLoaderResult>>(localDataProvider->loadSpriteAsync(scale));
    } else {
        textureLoaderFuture = std::make_shared<::djinni::Future<::TextureLoaderResult>>(LoaderHelper::loadTextureAsync(urlTexture, std::nullopt, loaders));
    }

    textureLoaderFuture->then([context] (auto result) {
        context->textureResult = std::make_shared<TextureLoaderResult>(result.get());

        if (--(context->counter) == 0) {
            context->promise.setValue();
        }
    });

    auto selfActor = WeakActor<Tiled2dMapVectorLayer>(mailbox, castedMe);
    context->promise.getFuture().then([context, selfActor, fromLocal, weakSelf, scale] (auto result) {
        auto jsonResultStatus = context->jsonResult->status;
        auto textureResultStatus = context->textureResult->status;

        if (scale == 3 && (jsonResultStatus != LoaderStatus::OK || textureResultStatus != LoaderStatus::OK)) {
            LogInfo <<= "This device would benefit from @3x assets, but none could be found. Please add @3x assets for crispy icons!";
            // 3@x assets are not available, so we try @2x
            auto self = weakSelf.lock();
            if (self) {
                self->loadSpriteData(2, fromLocal);
                return;
            }
        }
        
        std::shared_ptr<SpriteData> jsonData;
        std::shared_ptr<::TextureHolderInterface> spriteTexture;
        
        if (jsonResultStatus == LoaderStatus::OK) {
            auto string = std::string((char*)context->jsonResult->data->buf(), context->jsonResult->data->len());
            nlohmann::json json;
            try
            {
                json = nlohmann::json::parse(string);

                std::unordered_map<std::string, SpriteDesc> sprites;

                for (auto& [key, val] : json.items())
                {
                    sprites.insert({key, val.get<SpriteDesc>()});
                }

                jsonData = std::make_shared<SpriteData>(sprites);
            }
            catch (nlohmann::json::parse_error& ex)
            {
                LogError <<= ex.what();
            }
        }
        
        if (textureResultStatus == LoaderStatus::OK) {
            spriteTexture = context->textureResult->data;
        }
       
        if (!jsonData && !spriteTexture && fromLocal) {
            auto self = weakSelf.lock();
            if (self) {
                self->loadSpriteData(scale, false);
                return;
            }
        }
        
        
        selfActor.message(&Tiled2dMapVectorLayer::didLoadSpriteData, jsonData, spriteTexture);
    });
}

void Tiled2dMapVectorLayer::didLoadSpriteData(std::shared_ptr<SpriteData> spriteData, std::shared_ptr<::TextureHolderInterface> spriteTexture) {
    this->spriteData = spriteData;
    this->spriteTexture = spriteTexture;

    for (const auto &[source, manager] : symbolSourceDataManagers) {
        manager.message(&Tiled2dMapVectorSourceSymbolDataManager::setSprites, spriteData, spriteTexture);
    }

    for (const auto &[source, manager] : sourceDataManagers) {
        manager.message(&Tiled2dMapVectorSourceTileDataManager::setSprites, spriteData, spriteTexture);
    }

    if (backgroundLayer) {
        backgroundLayer->setSprites(spriteData, spriteTexture);
    }
}

void Tiled2dMapVectorLayer::setScissorRect(const std::optional<::RectI> &scissorRect) {
    this->scissorRect = scissorRect;
    pregenerateRenderPasses();
    auto mapInterface = this->mapInterface;
    if (mapInterface) {
        mapInterface->invalidate();
    }
}

void Tiled2dMapVectorLayer::setSelectionDelegate(const std::weak_ptr<Tiled2dMapVectorLayerSelectionCallbackInterface> &selectionDelegate) {
    this->selectionDelegate = selectionDelegate;
    for (const auto &[source, sourceDataManager]: sourceDataManagers) {
        sourceDataManager.message(&Tiled2dMapVectorSourceTileDataManager::setSelectionDelegate, selectionDelegate);
    }
    for (const auto &[source, sourceDataManager]: symbolSourceDataManagers) {
        sourceDataManager.message(&Tiled2dMapVectorSourceSymbolDataManager::setSelectionDelegate, selectionDelegate);
    }
}

void Tiled2dMapVectorLayer::setSelectionDelegate(const std::shared_ptr<Tiled2dMapVectorLayerSelectionCallbackInterface> &selectionDelegate) {
    this->strongSelectionDelegate = selectionDelegate;
    this->selectionDelegate = selectionDelegate;
    setSelectionDelegate(std::weak_ptr<Tiled2dMapVectorLayerSelectionCallbackInterface>(selectionDelegate));
}

void Tiled2dMapVectorLayer::updateLayerDescriptions(const std::vector<std::shared_ptr<VectorLayerDescription>> &layerDescriptions) {

    struct UpdateVector {
        std::vector<Tiled2dMapVectorLayerUpdateInformation> symbolUpdates;
        std::vector<Tiled2dMapVectorLayerUpdateInformation> updates;
    };

    std::map<std::string, UpdateVector> updateInformationsMap;

    for (const auto layerDescription: layerDescriptions) {
        std::shared_ptr<VectorLayerDescription> legacyDescription;
        int32_t legacyIndex = -1;
        {
            std::lock_guard<std::recursive_mutex> lock(mapDescriptionMutex);
            size_t numLayers = mapDescription->layers.size();
            for (int index = 0; index < numLayers; index++) {
                if (mapDescription->layers[index]->identifier == layerDescription->identifier) {
                    legacyDescription = mapDescription->layers[index];
                    legacyIndex = index;
                    mapDescription->layers[index] = layerDescription;
                    break;
                }
            }
        }


        if (legacyIndex < 0) {
            return;
        }

        auto legacySource = legacyDescription->source;
        auto newSource = layerDescription->source;

        // Evaluate if a complete replacement of the tiles is needed (source/zoom adjustments may lead to a different set of created tiles)
        bool needsTileReplace = legacyDescription->source != layerDescription->source
        || legacyDescription->sourceLayer != layerDescription->sourceLayer
        || legacyDescription->minZoom != layerDescription->minZoom
        || legacyDescription->maxZoom != layerDescription->maxZoom
        || !((legacyDescription->filter == nullptr && layerDescription->filter == nullptr ) || (legacyDescription->filter && legacyDescription->filter->isEqual(layerDescription->filter)));

        auto existing = updateInformationsMap.find(layerDescription->source);
        if (existing != updateInformationsMap.end()) {
            if (layerDescription->getType() == VectorLayerType::symbol) {
                existing->second.symbolUpdates.push_back({layerDescription, legacyDescription, legacyIndex, needsTileReplace});
            } else {
                existing->second.updates.push_back({layerDescription, legacyDescription, legacyIndex, needsTileReplace});
            }
        } else {
            if (layerDescription->getType() == VectorLayerType::symbol) {
                updateInformationsMap.insert({ layerDescription->source, {{{layerDescription, legacyDescription, legacyIndex, needsTileReplace}}, {}}});
            } else {
                updateInformationsMap.insert({ layerDescription->source, {{}, {{layerDescription, legacyDescription, legacyIndex, needsTileReplace}}}});
            }
        }
    }

    for (const auto [updateSource, updateInformations]: updateInformationsMap) {
        if (!updateInformations.symbolUpdates.empty()) {
            for (const auto &[source, sourceDataManager]: symbolSourceDataManagers) {
                if (updateSource == source) {
                    sourceDataManager.syncAccess([&objects = updateInformations.symbolUpdates] (const auto &manager) {
                        manager->updateLayerDescriptions(objects);
                    });
                }
            }
        } else if (!updateInformations.updates.empty()) {
            for (const auto &[source, sourceDataManager]: sourceDataManagers) {
                if (updateSource == source) {
                    sourceDataManager.syncAccess([&objects = updateInformations.updates] (const auto &manager) {
                        manager->updateLayerDescriptions(objects);
                    });
                }
            }
        }
    }

    tilesStillValid.clear();
    mapInterface->invalidate();
}

void Tiled2dMapVectorLayer::updateLayerDescription(std::shared_ptr<VectorLayerDescription> layerDescription) {
    std::shared_ptr<VectorLayerDescription> legacyDescription;
    int32_t legacyIndex = -1;
    {
        std::lock_guard<std::recursive_mutex> lock(mapDescriptionMutex);
        size_t numLayers = mapDescription->layers.size();
        for (int index = 0; index < numLayers; index++) {
            if (mapDescription->layers[index]->identifier == layerDescription->identifier) {
                legacyDescription = mapDescription->layers[index];
                legacyIndex = index;
                mapDescription->layers[index] = layerDescription;
                break;
            }
        }
    }


    if (legacyIndex < 0) {
        return;
    }

    auto legacySource = legacyDescription->source;
    auto newSource = layerDescription->source;

    // Evaluate if a complete replacement of the tiles is needed (source/zoom adjustments may lead to a different set of created tiles)
    bool needsTileReplace = legacyDescription->source != layerDescription->source
                            || legacyDescription->sourceLayer != layerDescription->sourceLayer
                            || legacyDescription->minZoom != layerDescription->minZoom
                            || legacyDescription->maxZoom != layerDescription->maxZoom
                            || !((legacyDescription->filter == nullptr && layerDescription->filter == nullptr ) || (legacyDescription->filter && legacyDescription->filter->isEqual(layerDescription->filter)));

    if (layerDescription->getType() == VectorLayerType::symbol) {
        for (const auto &[source, sourceDataManager]: symbolSourceDataManagers) {
            if (legacySource == source || newSource == source) {
                sourceDataManager.message(&Tiled2dMapVectorSourceDataManager::updateLayerDescription, layerDescription, legacyIndex,
                                          needsTileReplace);
            }
        }
    } else {
        for (const auto &[source, sourceDataManager]: sourceDataManagers) {
            if (legacySource == source || newSource == source) {
                sourceDataManager.message(&Tiled2dMapVectorSourceDataManager::updateLayerDescription, layerDescription, legacyIndex,
                                          needsTileReplace);
            }
        }
    }

    tilesStillValid.clear();
    mapInterface->invalidate();
}

std::optional<std::shared_ptr<FeatureContext>> Tiled2dMapVectorLayer::getFeatureContext(int64_t identifier) {
    for (const auto &[source, vectorTileSource] : vectorTileSources) {
        auto const &currentTileInfos = vectorTileSource.converse(&Tiled2dMapVectorSource::getCurrentTiles).get();

        for (auto const &tile: currentTileInfos) {
            for (auto it = tile.layerFeatureMaps->begin(); it != tile.layerFeatureMaps->end(); it++) {
                for (auto const &[featureContext, geometry]: *it->second) {
                    if (featureContext->identifier == identifier) {
                        return featureContext;
                    }
                }
            }
        }
    }
    
    return std::nullopt;
}

std::shared_ptr<VectorLayerDescription> Tiled2dMapVectorLayer::getLayerDescriptionWithIdentifier(std::string identifier) {
    std::lock_guard<std::recursive_mutex> lock(mapDescriptionMutex);
    if (mapDescription) {
        auto it = std::find_if(mapDescription->layers.begin(), mapDescription->layers.end(),
            [&identifier](const auto& layer) {
                return layer->identifier == identifier;
            });

        if (it != mapDescription->layers.end()) {
            return (*it)->clone();
        }
    }
    return nullptr;
}

// Touch Interface
bool Tiled2dMapVectorLayer::onTouchDown(const Vec2F &posScreen) {
    return interactionManager->onTouchDown(posScreen);
}

bool Tiled2dMapVectorLayer::onClickUnconfirmed(const Vec2F &posScreen) {
    return interactionManager->onClickUnconfirmed(posScreen);
}

bool Tiled2dMapVectorLayer::onClickConfirmed(const Vec2F &posScreen) {
    if (interactionManager->onClickConfirmed(posScreen)) {
        return true;
    }
    auto mapInterface = this->mapInterface;
    auto camera = mapInterface ? mapInterface->getCamera() : nullptr;
    if (!camera) {
        return false;
    }
    if(strongSelectionDelegate) {
        return strongSelectionDelegate->didClickBackgroundConfirmed(camera->coordFromScreenPosition(posScreen));
    } else if (auto ptr = selectionDelegate.lock()) {
        return ptr->didClickBackgroundConfirmed(camera->coordFromScreenPosition(posScreen));
    }
    return false;
}

void Tiled2dMapVectorLayer::performClick(const Coord &coord) {
    const auto mapInterface = this->mapInterface;
    const auto camera = mapInterface ? mapInterface->getCamera() : nullptr;

    if (!camera) {
        return;
    }
    
    const auto screenPos = camera->screenPosFromCoord(coord);
    onClickConfirmed(screenPos);
}

bool Tiled2dMapVectorLayer::onDoubleClick(const Vec2F &posScreen) {
    return interactionManager->onDoubleClick(posScreen);
}

bool Tiled2dMapVectorLayer::onLongPress(const Vec2F &posScreen) {
    return interactionManager->onLongPress(posScreen);
}

bool Tiled2dMapVectorLayer::onMove(const Vec2F &deltaScreen, bool confirmed, bool doubleClick) {
    return interactionManager->onMove(deltaScreen, confirmed, doubleClick);
}

bool Tiled2dMapVectorLayer::onMoveComplete() {
    return interactionManager->onMoveComplete();
}

bool Tiled2dMapVectorLayer::onTwoFingerClick(const Vec2F &posScreen1, const Vec2F &posScreen2) {
    return interactionManager->onTwoFingerClick(posScreen1, posScreen2);
}

bool Tiled2dMapVectorLayer::onTwoFingerMove(const std::vector<::Vec2F> &posScreenOld, const std::vector<::Vec2F> &posScreenNew) {
    return interactionManager->onTwoFingerMove(posScreenOld, posScreenNew);
}

bool Tiled2dMapVectorLayer::onTwoFingerMoveComplete() {
    return interactionManager->onTwoFingerMoveComplete();
}

void Tiled2dMapVectorLayer::clearTouch() {
    return interactionManager->clearTouch();
}

std::optional<std::string> Tiled2dMapVectorLayer::getStyleMetadataJson() {
    return metadata;
}

void Tiled2dMapVectorLayer::setFeatureState(const std::string & identifier, const std::unordered_map<std::string, VectorLayerFeatureInfoValue> & properties) {
    featureStateManager->setFeatureState(identifier, properties);
    applyGlobalOrFeatureStateIfPossible(StateType::FEATURE);
}

void Tiled2dMapVectorLayer::setGlobalState(const std::unordered_map<std::string, VectorLayerFeatureInfoValue> &properties) {
    featureStateManager->setGlobalState(properties);
    applyGlobalOrFeatureStateIfPossible(StateType::GLOBAL);
}

void Tiled2dMapVectorLayer::applyGlobalOrFeatureStateIfPossible(StateType type) {
    std::lock_guard<std::recursive_mutex> lock(mapDescriptionMutex);
    auto mapInterface = this->mapInterface;
    auto mapDescription = this->mapDescription;
    if(!mapInterface || !mapDescription) { return; }

    std::unordered_map<std::string, std::vector<std::tuple<std::string, std::string>>> sourceLayerIdentifiersMap;
    std::unordered_map<std::string, std::vector<std::tuple<std::shared_ptr<VectorLayerDescription>, int32_t>>> sourcelayerDescriptionIndexMap;
    int32_t layerIndex = -1;
    for (const auto &layerDescription : mapDescription->layers) {
        layerIndex++;
        if (!layerDescription->filter) {
            continue;
        }
        const auto &usedKeys = layerDescription->filter->getUsedKeys();
        if (((type == StateType::GLOBAL || type == StateType::BOTH) && !usedKeys.globalStateKeys.empty()) || ((type == StateType::FEATURE ||type == StateType::BOTH) && !usedKeys.featureStateKeys.empty())) {
            if (layerDescription->getType() == VectorLayerType::symbol) {
                sourceLayerIdentifiersMap[layerDescription->source].emplace_back(layerDescription->sourceLayer,
                                                                                 layerDescription->identifier);
            } else {
                sourcelayerDescriptionIndexMap[layerDescription->source].push_back({layerDescription, layerIndex});
            }
        }
    }

    for (const auto &[source, sourceLayerIdentifiers]: sourceLayerIdentifiersMap) {
        const auto &symbolManager = symbolSourceDataManagers.find(source);
        if (symbolManager != symbolSourceDataManagers.end()) {
            symbolManager->second.message(&Tiled2dMapVectorSourceSymbolDataManager::reloadLayerContent, sourceLayerIdentifiers);
        }
    }
    for (const auto &[source, descriptionIndexPairs]: sourcelayerDescriptionIndexMap) {
        const auto &dataManager = sourceDataManagers.find(source);
        if (dataManager != sourceDataManagers.end()) {
            dataManager->second.message(&Tiled2dMapVectorSourceTileDataManager::reloadLayerContent, descriptionIndexPairs);
        }
    }

    tilesStillValid.clear();
    mapInterface->invalidate();
}

LayerReadyState Tiled2dMapVectorLayer::isReadyToRenderOffscreen() {
    if (layerConfigs.empty() || sourceInterfaces.empty()) {
        return LayerReadyState::NOT_READY;
    }
    return Tiled2dMapLayer::isReadyToRenderOffscreen();
}

void Tiled2dMapVectorLayer::enableAnimations(bool enabled) {
    this->animationsEnabled = enabled;
    for (const auto &[source, manager] : symbolSourceDataManagers) {
        manager.syncAccess([enabled](const std::shared_ptr<Tiled2dMapVectorSourceSymbolDataManager> manager) {
            manager->enableAnimations(enabled);
        });
    }
}

void Tiled2dMapVectorLayer::setMinZoomLevelIdentifier(std::optional<int32_t> value) {
    Tiled2dMapLayer::setMinZoomLevelIdentifier(value);
}

std::optional<int32_t> Tiled2dMapVectorLayer::getMinZoomLevelIdentifier() {
    return Tiled2dMapLayer::getMinZoomLevelIdentifier();
}

void Tiled2dMapVectorLayer::setMaxZoomLevelIdentifier(std::optional<int32_t> value) {
    Tiled2dMapLayer::setMaxZoomLevelIdentifier(value);
}

std::optional<int32_t> Tiled2dMapVectorLayer::getMaxZoomLevelIdentifier() {
    return Tiled2dMapLayer::getMaxZoomLevelIdentifier();
}

void Tiled2dMapVectorLayer::invalidateCollisionState() {
    prevCollisionStillValid.clear();
    tilesStillValid.clear();
    if (mapInterface) {
        mapInterface->invalidate();
    }
}

void Tiled2dMapVectorLayer::setReadyStateListener(const /*not-null*/ std::shared_ptr<::Tiled2dMapReadyStateListener> & listener) {
    readyStateListener = listener;
}

void Tiled2dMapVectorLayer::updateReadyStateListenerIfNeeded() {
    const auto listener = readyStateListener;
    if (!listener) {
        return;
    }
    const auto newState = isReadyToRenderOffscreen();
    if (newState != lastReadyState) {
        listener->stateUpdate(newState);
        lastReadyState = newState;
    }
}

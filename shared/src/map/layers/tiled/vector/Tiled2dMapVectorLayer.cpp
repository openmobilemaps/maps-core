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
#include "OBB2D.h"
#include "DataLoaderResult.h"
#include "TextureLoaderResult.h"
#include "PolygonCompare.h"
#include "LoaderHelper.h"
#include "Tiled2dMapVectorPolygonTile.h"
#include "Tiled2dMapVectorLineTile.h"
#include "Tiled2dMapVectorRasterTile.h"
#include "SpriteData.h"
#include "Tiled2dMapVectorBackgroundSubLayer.h"
#include "Tiled2dMapVectorSourceTileDataManager.h"
#include "Tiled2dMapVectorSourceRasterTileDataManager.h"
#include "Tiled2dMapVectorSourceVectorTileDataManager.h"
#include "Tiled2dMapVectorSourceSymbolDataManager.h"
#include "Tiled2dMapVectorSourceSymbolCollisionManager.h"
#include "Tiled2dMapVectorInteractionManager.h"

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
                auto sourceMailbox = std::make_shared<Mailbox>(mapInterface->getScheduler());
                auto sourceActor = Actor<Tiled2dMapRasterSource>(sourceMailbox,
                                                                 mapInterface->getMapConfig(),
                                                                 std::make_shared<Tiled2dMapVectorRasterSubLayerConfig>(
                                                                         std::static_pointer_cast<RasterVectorLayerDescription>(
                                                                                 layerDesc)),
                                                                 mapInterface->getCoordinateConverterHelper(),
                                                                 mapInterface->getScheduler(), loaders,
                                                                 selfRasterActor,
                                                                 mapInterface->getCamera()->getScreenDensityPpi());
                rasterSources.push_back(sourceActor);
                auto sourceDataManagerMailbox = std::make_shared<Mailbox>(mapInterface->getScheduler());
                auto sourceManagerActor = Actor<Tiled2dMapVectorSourceRasterTileDataManager>(sourceDataManagerMailbox,
                                                                                             selfActor,
                                                                                             mapDescription,
                                                                                             layerDesc->source,
                                                                                             sourceActor.weakActor<Tiled2dMapRasterSource>());
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
                layersToDecode[layerDesc->source].insert(layerDesc->sourceId);
                break;
            }
        }
    }

    for (auto const &[source, layers]: layersToDecode) {
        auto sourceMailbox = std::make_shared<Mailbox>(mapInterface->getScheduler());
        auto vectorSource = Actor<Tiled2dMapVectorSource>(sourceMailbox,
                                                          mapInterface->getMapConfig(),
                                                          layerConfigs[source],
                                                          mapInterface->getCoordinateConverterHelper(),
                                                          mapInterface->getScheduler(),
                                                          loaders,
                                                          selfVectorActor,
                                                          layers,
                                                          source,
                                                          mapInterface->getCamera()->getScreenDensityPpi());
        vectorTileSources[source] = vectorSource;
        sourceInterfaces.push_back(vectorSource.weakActor<Tiled2dMapSourceInterface>());
        auto sourceDataManagerMailbox = std::make_shared<Mailbox>(mapInterface->getScheduler());
        auto sourceManagerActor = Actor<Tiled2dMapVectorSourceVectorTileDataManager>(sourceDataManagerMailbox,
                                                                                     selfActor,
                                                                                     mapDescription,
                                                                                     source,
                                                                                     vectorSource.weakActor<Tiled2dMapVectorSource>());
        sourceTileManagers[source] = sourceManagerActor.strongActor<Tiled2dMapVectorSourceTileDataManager>();
        interactionDataManagers[source].push_back(sourceManagerActor.weakActor<Tiled2dMapVectorSourceDataManager>());

        if (symbolSources.count(source) != 0) {
            auto symbolSourceDataManagerMailbox = std::make_shared<Mailbox>(mapInterface->getScheduler());
            auto actor = Actor<Tiled2dMapVectorSourceSymbolDataManager>(symbolSourceDataManagerMailbox,
                                                                        selfActor,
                                                                        mapDescription,
                                                                        source,
                                                                        fontLoader,
                                                                        vectorSource.weakActor<Tiled2dMapVectorSource>());
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

    if (selectionDelegate) {
        setSelectionDelegate(selectionDelegate);
    }

    if (selectedFeatureIdentifier) {
        setSelectedFeatureIdentifier(selectedFeatureIdentifier);
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
        for (const auto &[source, vectorTileSource] : vectorTileSources) {
            vectorTileSource.message(&Tiled2dMapVectorSource::resume);
        }

        for (const auto &rasterSource : rasterSources) {
            rasterSource.message(&Tiled2dMapRasterSource::resume);
        }
    }
}

std::shared_ptr<::LayerInterface> Tiled2dMapVectorLayer::asLayerInterface() {
    return shared_from_this();
}

void Tiled2dMapVectorLayer::update() {
    for (const auto &[source, sourceDataManager]: sourceDataManagers) {
        sourceDataManager.message(&Tiled2dMapVectorSourceTileDataManager::update);
    }

    if (collisionManager) {
        collisionManager.syncAccess([](const auto &manager) {
            manager->update();
        });
        collisionManager.message(MailboxDuplicationStrategy::replaceNewest, &Tiled2dMapVectorSourceSymbolCollisionManager::collisionDetection);
    }
}

std::vector<std::shared_ptr<::RenderPassInterface>> Tiled2dMapVectorLayer::buildRenderPasses() {
    std::lock_guard<std::recursive_mutex> lock(renderPassMutex);
    return currentRenderPasses;
}

void Tiled2dMapVectorLayer::onRenderPassUpdate(const std::string &source, bool isSymbol, const std::vector<std::shared_ptr<TileRenderDescription>> &renderDescription) {
    std::vector<std::shared_ptr<RenderPassInterface>> newPasses;

    if (isSymbol) {
        sourceRenderDescriptionMap[source].symbolRenderDescriptions = renderDescription;
    } else {
        sourceRenderDescriptionMap[source].renderDescriptions = renderDescription;
    }

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

    for (const auto &description : orderedRenderDescriptions) {
        if (description->maskingObject != lastMask && !renderObjects.empty()) {
            newPasses.emplace_back(std::make_shared<RenderPass>(RenderPassConfig(0), renderObjects, lastMask));
            renderObjects.clear();
            lastMask = nullptr;
        }

        if (description->isModifyingMask) {
            if (!renderObjects.empty()) {
                newPasses.emplace_back(std::make_shared<RenderPass>(RenderPassConfig(0), renderObjects, lastMask));
            }
            renderObjects.clear();
            lastMask = nullptr;
            newPasses.emplace_back(std::make_shared<RenderPass>(RenderPassConfig(0), description->renderObjects, description->maskingObject));
        } else {
            renderObjects.insert(renderObjects.end(), description->renderObjects.begin(), description->renderObjects.end());
            lastMask = description->maskingObject;
        }
    }
    if (!renderObjects.empty()) {
        newPasses.emplace_back(std::make_shared<RenderPass>(RenderPassConfig(0), renderObjects, lastMask));
        renderObjects.clear();
        lastMask = nullptr;
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
        sourceDataManager.message(MailboxExecutionEnvironment::graphics, &Tiled2dMapVectorSourceTileDataManager::resume);
    }
    for (const auto &[source, sourceDataManager]: symbolSourceDataManagers) {
        sourceDataManager.message(MailboxExecutionEnvironment::graphics, &Tiled2dMapVectorSourceSymbolDataManager::resume);
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
    if (!isLoadingStyleJson && remoteStyleJsonUrl.has_value() && !mapDescription && !vectorTileSources.empty()) {
        scheduleStyleJsonLoading();
        return;
    }
    Tiled2dMapLayer::forceReload();
}

void Tiled2dMapVectorLayer::onTilesUpdated(const std::string &layerName, std::unordered_set<Tiled2dMapRasterTileInfo> currentTileInfos) {
    for (const auto &[source, sourceDataManager]: sourceDataManagers) {
        sourceDataManager.message(MailboxDuplicationStrategy::replaceNewest, &Tiled2dMapVectorSourceTileDataManager::onRasterTilesUpdated, layerName, currentTileInfos);
    }
}

void Tiled2dMapVectorLayer::onTilesUpdated(const std::string &sourceName, std::unordered_set<Tiled2dMapVectorTileInfo> currentTileInfos) {
    auto sourceManager = sourceDataManagers.find(sourceName);
    if (sourceManager != sourceDataManagers.end()) {
        sourceManager->second.message(MailboxDuplicationStrategy::replaceNewest, &Tiled2dMapVectorSourceTileDataManager::onVectorTilesUpdated, sourceName, currentTileInfos);
    }
    auto symbolSourceManager = symbolSourceDataManagers.find(sourceName);
    if (symbolSourceManager != symbolSourceDataManagers.end()) {
        symbolSourceManager->second.message(MailboxDuplicationStrategy::replaceNewest, &Tiled2dMapVectorSourceTileDataManager::onVectorTilesUpdated, sourceName, currentTileInfos);
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

    struct Context {
        std::atomic<size_t> counter;

        std::shared_ptr<SpriteData> spriteData;
        std::shared_ptr<::TextureHolderInterface> spriteTexture;

        djinni::Promise<void> promise;
        Context(size_t c) : counter(c) {}
    };

    auto context = std::make_shared<Context>(2);

    LoaderHelper::loadDataAsync(urlData, std::nullopt, loaders).then([context] (auto result) {
        auto dataResult = result.get();
        if (dataResult.status == LoaderStatus::OK) {
            auto string = std::string((char*)dataResult.data->buf(), dataResult.data->len());
            nlohmann::json json;
            try
            {
                json = nlohmann::json::parse(string);

                std::unordered_map<std::string, SpriteDesc> sprites;

                for (auto& [key, val] : json.items())
                {
                    sprites.insert({key, val.get<SpriteDesc>()});
                }

                context->spriteData = std::make_shared<SpriteData>(sprites);
            }
            catch (nlohmann::json::parse_error& ex)
            {
                LogError <<= ex.what();
            }
        }

        if (--(context->counter) == 0) {
            context->promise.setValue();
        }
    });

    LoaderHelper::loadTextureAsync(urlTexture, std::nullopt, loaders).then([context] (auto result) {
        auto textureResult = result.get();
        if (textureResult.status == LoaderStatus::OK) {
            context->spriteTexture = textureResult.data;
        }

        if (--(context->counter) == 0) {
            context->promise.setValue();
        }
    });

    auto castedMe = std::static_pointer_cast<Tiled2dMapVectorLayer>(shared_from_this());
    auto selfActor = WeakActor<Tiled2dMapVectorLayer>(mailbox, castedMe);
    context->promise.getFuture().then([context, selfActor] (auto result) {
        selfActor.message(&Tiled2dMapVectorLayer::didLoadSpriteData, context->spriteData, context->spriteTexture);
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
}

void Tiled2dMapVectorLayer::setScissorRect(const std::optional<::RectI> &scissorRect) {
    this->scissorRect = scissorRect;
    for (const auto &[source, sourceDataManager]: sourceDataManagers) {
        sourceDataManager.message(&Tiled2dMapVectorSourceDataManager::setScissorRect, scissorRect);
    }
    for (const auto &[source, sourceDataManager]: symbolSourceDataManagers) {
        sourceDataManager.message(&Tiled2dMapVectorSourceDataManager::setScissorRect, scissorRect);
    }
    auto mapInterface = this->mapInterface;
    if (mapInterface) {
        mapInterface->invalidate();
    }
}

void Tiled2dMapVectorLayer::setSelectionDelegate(const std::shared_ptr<Tiled2dMapVectorLayerSelectionCallbackInterface> &selectionDelegate) {
    this->selectionDelegate = selectionDelegate;

    for (const auto &[source, sourceDataManager]: sourceDataManagers) {
        sourceDataManager.message(&Tiled2dMapVectorSourceTileDataManager::setSelectionDelegate, selectionDelegate);
    }
    for (const auto &[source, sourceDataManager]: symbolSourceDataManagers) {
        sourceDataManager.message(&Tiled2dMapVectorSourceSymbolDataManager::setSelectionDelegate, selectionDelegate);
    }
}

void Tiled2dMapVectorLayer::setSelectedFeatureIdentifier(std::optional<int64_t> identifier) {
    this->selectedFeatureIdentifier = identifier;
    for (const auto &[source, sourceDataManager]: sourceDataManagers) {
        sourceDataManager.message(MailboxDuplicationStrategy::replaceNewest, &Tiled2dMapVectorSourceTileDataManager::setSelectedFeatureIdentifier, identifier);
    }
    for (const auto &[source, sourceDataManager]: symbolSourceDataManagers) {
        sourceDataManager.message(MailboxDuplicationStrategy::replaceNewest, &Tiled2dMapVectorSourceSymbolDataManager::setSelectedFeatureIdentifier, identifier);
    }
    auto mapInterface = this->mapInterface;
    if (mapInterface) {
        mapInterface->invalidate();
    }
}

void Tiled2dMapVectorLayer::updateLayerDescription(std::shared_ptr<VectorLayerDescription> layerDescription) {
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

    auto legacySource = legacyDescription->source;
    auto newSource = layerDescription->source;

    // Evaluate if a complete replacement of the tiles is needed (source/zoom adjustments may lead to a different set of created tiles)
    bool needsTileReplace = legacyDescription->source != layerDescription->source
                            || legacyDescription->sourceId != layerDescription->sourceId
                            || legacyDescription->minZoom != layerDescription->minZoom
                            || legacyDescription->maxZoom != layerDescription->maxZoom
                            || legacyDescription->filter != layerDescription->filter;

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
    return interactionManager->onClickConfirmed(posScreen);
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

/*
 * Copyright (c) 2021 Ubique Innovation AG <https://www.ubique.ch>
 *
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this
 *  file, You can obtain one at https://mozilla.org/MPL/2.0/.
 *
 *  SPDX-License-Identifier: MPL-2.0
 */

#include "Tiled2dMapVectorSourceSymbolDataManager.h"
#include "Tiled2dMapVectorLayer.h"
#include "Tiled2dMapVectorLayerConfig.h"
#include "RenderPass.h"
#include "Matrix.h"
#include "StretchShaderInfo.h"
#include "Quad2dInstancedInterface.h"
#include "RenderObject.h"

Tiled2dMapVectorSourceSymbolDataManager::Tiled2dMapVectorSourceSymbolDataManager(const WeakActor<Tiled2dMapVectorLayer> &vectorLayer,
                                                                                 const std::shared_ptr<VectorMapDescription> &mapDescription,
                                                                                 const std::shared_ptr<Tiled2dMapVectorLayerConfig> &layerConfig,
                                                                                 const std::string &source,
                                                                                 const std::shared_ptr<FontLoaderInterface> &fontLoader,
                                                                                 const WeakActor<Tiled2dMapVectorSource> &vectorSource,
                                                                                 const Actor<Tiled2dMapVectorReadyManager> &readyManager,
                                                                                 const std::shared_ptr<Tiled2dMapVectorFeatureStateManager> &featureStateManager) :
Tiled2dMapVectorSourceDataManager(vectorLayer, mapDescription, layerConfig, source, readyManager, featureStateManager), fontLoader(fontLoader), vectorSource(vectorSource), animationCoordinatorMap(std::make_shared<SymbolAnimationCoordinatorMap>())
{

    readyManager.message(&Tiled2dMapVectorReadyManager::registerManager);

    for (const auto &layer: mapDescription->layers) {
        if (layer->getType() == VectorLayerType::symbol && layer->source == source) {
            layerDescriptions.insert({layer->identifier, std::static_pointer_cast<SymbolVectorLayerDescription>(layer)});
            if (layer->isInteractable(EvaluationContext(0.0, std::make_shared<FeatureContext>(), featureStateManager))) {
                interactableLayers.insert(layer->identifier);
            }
        }
    }
}

void Tiled2dMapVectorSourceSymbolDataManager::onAdded(const std::weak_ptr< ::MapInterface> &mapInterface) {
    Tiled2dMapVectorSourceDataManager::onAdded(mapInterface);
    textHelper.setMapInterface(mapInterface);
}

void Tiled2dMapVectorSourceSymbolDataManager::onRemoved() {
    Tiled2dMapVectorSourceDataManager::onRemoved();
    // Clear (on GLThread!) and remove all symbol groups - they're dependant on the mapInterface
}

void Tiled2dMapVectorSourceSymbolDataManager::pause() {
    for (const auto &[tileInfo, tileSymbolGroups]: tileSymbolGroupMap) {
        for (const auto &[s, symbolGroups]: tileSymbolGroups) {
            for (const auto &symbolGroup: symbolGroups) {
                symbolGroup.syncAccess([&](auto group){
                    group->clear();
                });
            }
        }
    }
}

void Tiled2dMapVectorSourceSymbolDataManager::resume() {
    auto mapInterface = this->mapInterface.lock();
    const auto &context = mapInterface ? mapInterface->getRenderingContext() : nullptr;
    if (!context) {
        return;
    }

    for (const auto &[tileInfo, tileSymbolGroups]: tileSymbolGroupMap) {
        for (const auto &[s, symbolGroups]: tileSymbolGroups) {
            for (const auto &symbolGroup: symbolGroups) {
                symbolGroup.syncAccess([&](auto group){
                    group->setupObjects(spriteData, spriteTexture);
                });
            }
        }
    }
}

void Tiled2dMapVectorSourceSymbolDataManager::setAlpha(float alpha) {
    this->alpha = alpha;
    for (const auto &[tileInfo, tileSymbolGroups]: tileSymbolGroupMap) {
        for (const auto &[s, symbolGroups]: tileSymbolGroups) {
            for (const auto &symbolGroup: symbolGroups) {
               symbolGroup.message(MailboxDuplicationStrategy::replaceNewest, &Tiled2dMapVectorSymbolGroup::setAlpha, alpha);
            }
        }
    }
}

void Tiled2dMapVectorSourceSymbolDataManager::updateLayerDescription(std::shared_ptr<VectorLayerDescription> layerDescription,
                                                                     int32_t legacyIndex,
                                                                     bool needsTileReplace) {
    auto mapInterface = this->mapInterface.lock();
    if (!mapInterface) {
        return;
    }

    if (layerDescription->source != source || layerDescription->getType() != VectorLayerType::symbol) {
        return;
    }

    auto oldStyle = layerDescriptions.at(layerDescription->identifier);
    auto castedDescription = std::static_pointer_cast<SymbolVectorLayerDescription>(layerDescription);

    bool iconWasAdded = false;

    if (oldStyle->style.hasIconImagePotentially() != castedDescription->style.hasIconImagePotentially() &&  oldStyle->style.hasIconImagePotentially() == false) {
        iconWasAdded = true;
    }

    layerDescriptions.erase(layerDescription->identifier);
    layerDescriptions.insert({layerDescription->identifier, castedDescription});

    if (needsTileReplace || iconWasAdded) {
        auto const &currentTileInfos = vectorSource.converse(&Tiled2dMapVectorSource::getCurrentTiles).get();

        std::unordered_map<Tiled2dMapTileInfo, std::vector<Actor<Tiled2dMapVectorSymbolGroup>>> toSetup;
        std::vector<Actor<Tiled2dMapVectorSymbolGroup>> toClear;

        for (const auto &tileData: currentTileInfos) {
            auto tileGroup = tileSymbolGroupMap.find(tileData.tileInfo);
            if (tileGroup == tileSymbolGroupMap.end()) {
                continue;
            }

            auto tileGroupIt = tileGroup->second.find(layerDescription->identifier);
            if (tileGroupIt != tileGroup->second.end()) {
                for (const auto &group : tileGroupIt->second) {
                    toClear.push_back(group);
                }
            }

            tileGroup->second.erase(layerDescription->identifier);

            // If new source of layer is not handled by this manager, continue
            if (layerDescription->source != source) {
                continue;
            }

            const auto &dataIt = tileData.layerFeatureMaps->find(layerDescription->sourceLayer);

            if (dataIt != tileData.layerFeatureMaps->end()) {
                // there is something in this layer to display
                const auto &newSymbolGroups = createSymbolGroups(tileData.tileInfo, layerDescription->identifier, dataIt->second);
                if (!newSymbolGroups.empty()) {
                    for (const auto &group : newSymbolGroups) {
                        tileSymbolGroupMap.at(tileData.tileInfo)[layerDescription->identifier].push_back(group);
                        toSetup[tileData.tileInfo].push_back(group);
                    }
                }
            }
        }

        auto selfActor = WeakActor(mailbox, weak_from_this());
        selfActor.message(MailboxExecutionEnvironment::graphics, &Tiled2dMapVectorSourceSymbolDataManager::setupSymbolGroups, toSetup,
                          toClear, std::unordered_set<Tiled2dMapTileInfo>{}, std::unordered_map<Tiled2dMapTileInfo, TileState>{});
    } else {
        for (const auto &[tileInfo, groupMap]: tileSymbolGroupMap) {
            auto const groupsIt = groupMap.find(layerDescription->identifier);
            if (groupsIt != groupMap.end()) {
                for (const auto &group: groupsIt->second) {
                    group.messagePrecisely(MailboxDuplicationStrategy::replaceNewest, MailboxExecutionEnvironment::graphics, &Tiled2dMapVectorSymbolGroup::updateLayerDescription, castedDescription);
                }
            }
        }

        mapInterface->invalidate();

    }
}

void Tiled2dMapVectorSourceSymbolDataManager::onVectorTilesUpdated(const std::string &sourceName,
                                                                   std::unordered_set<Tiled2dMapVectorTileInfo> currentTileInfos) {
    auto mapInterface = this->mapInterface.lock();
    auto graphicsFactory = mapInterface ? mapInterface->getGraphicsObjectFactory() : nullptr;
    auto coordinateConverterHelper = mapInterface ? mapInterface->getCoordinateConverterHelper() : nullptr;
    auto shaderFactory = mapInterface ? mapInterface->getShaderFactory() : nullptr;
    if (!graphicsFactory || !shaderFactory) {
        return;
    }

    // Just insert pointers here since we will only access the objects inside this method where we know that currentTileInfos is retained
    std::vector<const Tiled2dMapVectorTileInfo*> tilesToAdd;
    std::unordered_set<Tiled2dMapTileInfo> tilesToRemove;
    std::unordered_map<Tiled2dMapTileInfo, TileState> tileStateUpdates;

    for (const auto &vectorTileInfo: currentTileInfos) {
        if (tileSymbolGroupMap.count(vectorTileInfo.tileInfo) == 0) {
            tilesToAdd.push_back(&vectorTileInfo);
        } else {
            auto tileStateIt = tileStateMap.find(vectorTileInfo.tileInfo);
            if (tileStateIt == tileStateMap.end() || (tileStateIt->second != vectorTileInfo.state)) {
                tileStateUpdates[vectorTileInfo.tileInfo] = vectorTileInfo.state;
            }
        }
    }

    std::vector<Actor<Tiled2dMapVectorSymbolGroup>> toClear;
    for (const auto &[tileInfo, groupMap]: tileSymbolGroupMap) {
        bool found = false;
        for (const auto &currentTile: currentTileInfos) {
            if (tileInfo == currentTile.tileInfo) {
                found = true;
                break;
            }
        }
        if (!found) {
            tilesToRemove.insert(tileInfo);
            for (const auto &[_, groups] : groupMap) {
                for (const auto &group : groups) {
                    toClear.push_back(group);
                }
            }
        }
    }

    if (tilesToAdd.empty() && tilesToRemove.empty() && tileStateUpdates.empty()) {
        return;
    }

    std::unordered_map<Tiled2dMapTileInfo, std::vector<Actor<Tiled2dMapVectorSymbolGroup>>> toSetup;

    for (const auto &tile : tilesToAdd) {
        tileSymbolGroupMap[tile->tileInfo] = {};
        tileStateUpdates[tile->tileInfo] = tile->state;
        size_t notReadyCount = 0;

        for (const auto &[layerIdentifier, layer]: layerDescriptions) {
            const auto &dataIt = tile->layerFeatureMaps->find(layer->sourceLayer);
            if (dataIt != tile->layerFeatureMaps->end()) {
                // there is something in this layer to display
                const auto &newSymbolGroups = createSymbolGroups(tile->tileInfo, layerIdentifier, dataIt->second);
                if (!newSymbolGroups.empty()) {
                    for (const auto &group : newSymbolGroups) {
                        tileSymbolGroupMap.at(tile->tileInfo)[layerIdentifier].push_back(group);
                        toSetup[tile->tileInfo].push_back(group);
                        notReadyCount += 1;
                    }
                }
            }
        }

        readyManager.message(&Tiled2dMapVectorReadyManager::didProcessData, tile->tileInfo, notReadyCount);
    }

    auto selfActor = WeakActor(mailbox, weak_from_this());
    selfActor.message(MailboxExecutionEnvironment::graphics, &Tiled2dMapVectorSourceSymbolDataManager::setupSymbolGroups, toSetup, toClear, tilesToRemove, tileStateUpdates);
}

std::vector<Actor<Tiled2dMapVectorSymbolGroup>> Tiled2dMapVectorSourceSymbolDataManager::createSymbolGroups(const Tiled2dMapTileInfo &tileInfo,
                                                                                                                   const std::string &layerIdentifier,
                                                                                                                   const std::shared_ptr<std::vector<Tiled2dMapVectorTileInfo::FeatureTuple>> &features) {
    std::vector<Actor<Tiled2dMapVectorSymbolGroup>> symbolGroups = {};
    int32_t numFeatures = features ? (int32_t) features->size() : 0;
    for (int32_t featuresBase = 0; featuresBase < numFeatures; featuresBase += maxNumFeaturesPerGroup) {
        const auto fontProvider = WeakActor(mailbox, weak_from_this()).weakActor<Tiled2dMapVectorFontProvider>();
        auto mailbox = std::make_shared<Mailbox>(mapInterface.lock()->getScheduler());
        Actor<Tiled2dMapVectorSymbolGroup> symbolGroupActor = Actor<Tiled2dMapVectorSymbolGroup>(mailbox, mapInterface,
                                                                                                 layerConfig,
                                                                                                 fontProvider, tileInfo,
                                                                                                 layerIdentifier,
                                                                                                 layerDescriptions.at(
                                                                                                         layerIdentifier),
                                                                                                 featureStateManager);
        bool initialized = symbolGroupActor.unsafe()->initialize(features,
                                                                 featuresBase,
                                                                 std::min(featuresBase + maxNumFeaturesPerGroup, numFeatures) - featuresBase,
                                                                 animationCoordinatorMap);
        symbolGroupActor.unsafe()->setAlpha(alpha);
        if (initialized) {
            symbolGroups.push_back(symbolGroupActor);
        }
    }
    return symbolGroups;
}


void Tiled2dMapVectorSourceSymbolDataManager::setupSymbolGroups(const std::unordered_map<Tiled2dMapTileInfo, std::vector<Actor<Tiled2dMapVectorSymbolGroup>>> &toSetup,
                                                                const std::vector<Actor<Tiled2dMapVectorSymbolGroup>> &toClear,
                                                                const std::unordered_set<Tiled2dMapTileInfo> &tilesStatesToRemove,
                                                                const std::unordered_map<Tiled2dMapTileInfo, TileState> &tileStateUpdates) {
    auto mapInterface = this->mapInterface.lock();
    auto context = mapInterface ? mapInterface->getRenderingContext() : nullptr;
    if (!context) { return; }

    for (const auto &[tile, state]: tileStateUpdates) {
        auto tileStateIt = tileStateMap.find(tile);
        if (tileStateIt == tileStateMap.end()) {
            tileStateMap[tile] = state;
        } else {
            tileStateIt->second = state;
        }
        if (state == TileState::CACHED) {
            for (const auto &[layerIdentifier, symbolGroups]: tileSymbolGroupMap.at(tile)) {
                for (auto &symbolGroup: symbolGroups) {
                    symbolGroup.syncAccess([&](auto group){
                        group->placedInCache();
                    });
                }
            }
        }
    }

    for (const auto &tile: tilesStatesToRemove) {
        tileStateMap.erase(tile);
        tileSymbolGroupMap.erase(tile);
    }

    readyManager.syncAccess([tilesStatesToRemove](auto manager){
        manager->remove(tilesStatesToRemove);
    });

    for (const auto &symbolGroup: toClear) {
        symbolGroup.syncAccess([&](auto group){
            group->clear();
        });
    }

    for (const auto &[tile, groups]: toSetup) {
        for (const auto &symbolGroup: groups) {
            symbolGroup.syncAccess([&](auto group){
                group->setupObjects(spriteData, spriteTexture);
            });
        }
        readyManager.message(&Tiled2dMapVectorReadyManager::setReady, tile, groups.size());
    }

    if (!toClear.empty()) {
        animationCoordinatorMap->clearAnimationCoordinators();
    }

    for (const auto &[tile, symbolGroupMap]: tileSymbolGroupMap) {
        const auto tileState = tileStateMap.find(tile);
        if (tileState == tileStateMap.end() || tileState->second != TileState::VISIBLE) {
            continue;
        }
    }

    pregenerateRenderPasses();
}

std::shared_ptr<FontLoaderResult> Tiled2dMapVectorSourceSymbolDataManager::loadFont(const std::string &fontName) {
    if (fontLoaderResults.count(fontName) > 0) {
        return fontLoaderResults.at(fontName);
    } else {
        auto fontResult = std::make_shared<FontLoaderResult>(fontLoader->loadFont(Font(fontName)));
        if (fontResult->status == LoaderStatus::OK && fontResult->fontData && fontResult->imageData) {
            fontLoaderResults.insert({fontName, fontResult});
        }
        return fontResult;
    }
}

void Tiled2dMapVectorSourceSymbolDataManager::setSprites(std::shared_ptr<SpriteData> spriteData, std::shared_ptr<TextureHolderInterface> spriteTexture) {
    this->spriteData = spriteData;
    this->spriteTexture = spriteTexture;

    if (!tileSymbolGroupMap.empty()) {
        auto selfActor = WeakActor(mailbox, weak_from_this());
        selfActor.message(MailboxExecutionEnvironment::graphics, &Tiled2dMapVectorSourceSymbolDataManager::setupExistingSymbolWithSprite);
    }
}

void Tiled2dMapVectorSourceSymbolDataManager::setupExistingSymbolWithSprite() {
    auto mapInterface = this->mapInterface.lock();

    if (!mapInterface) {
        return;
    }

    for (const auto &[tile, symbolGroupMap]: tileSymbolGroupMap) {
        for (const auto &[layerIdentifier, symbolGroups]: symbolGroupMap) {
            for (auto &symbolGroup: symbolGroups) {
                symbolGroup.message(MailboxExecutionEnvironment::graphics, &Tiled2dMapVectorSymbolGroup::setupObjects, spriteData, spriteTexture);
            }
        }
    }
    
    pregenerateRenderPasses();
}

void Tiled2dMapVectorSourceSymbolDataManager::collisionDetection(std::vector<std::string> layerIdentifiers, std::shared_ptr<CollisionGrid> collisionGrid) {
    auto mapInterface = this->mapInterface.lock();
    auto camera = mapInterface ? mapInterface->getCamera() : nullptr;
    auto renderingContext = mapInterface ? mapInterface->getRenderingContext() : nullptr;
    auto converter = mapInterface ? mapInterface->getCoordinateConverterHelper() : nullptr;
    if (!camera || !converter || !renderingContext) {
        return;
    }

    double zoom = camera->getZoom();
    double zoomIdentifier = layerConfig->getZoomIdentifier(zoom);
    double rotation = -camera->getRotation();
    auto scaleFactor = camera->mapUnitsFromPixels(1.0);

    for (const auto layerIdentifier: layerIdentifiers) {
        for (const auto &[tile, symbolGroupsMap]: tileSymbolGroupMap) {
            const auto tileState = tileStateMap.find(tile);
            if (tileState == tileStateMap.end() || tileState->second != TileState::VISIBLE) {
                continue;
            }
            const auto objectsIt = symbolGroupsMap.find(layerIdentifier);
            if (objectsIt != symbolGroupsMap.end()) {
                for (auto &symbolGroup: objectsIt->second) {
                    symbolGroup.syncAccess([&zoomIdentifier, &rotation, &scaleFactor, &collisionGrid](auto group){
                        group->collisionDetection(zoomIdentifier, rotation, scaleFactor, collisionGrid);
                    });
                }
            }
        }
    }
}

void Tiled2dMapVectorSourceSymbolDataManager::update(long long now) {
    auto mapInterface = this->mapInterface.lock();
    auto camera = mapInterface ? mapInterface->getCamera() : nullptr;
    auto renderingContext = mapInterface ? mapInterface->getRenderingContext() : nullptr;
    auto converter = mapInterface ? mapInterface->getCoordinateConverterHelper() : nullptr;
    if (!camera || !converter || !renderingContext) {
        return;
    }

    const double zoomIdentifier = layerConfig->getZoomIdentifier(camera->getZoom());
    const double rotation = camera->getRotation();

    const auto scaleFactor = camera->mapUnitsFromPixels(1.0);

    for (const auto &[tile, symbolGroupsMap]: tileSymbolGroupMap) {
        const auto tileState = tileStateMap.find(tile);
        if (tileState == tileStateMap.end() || tileState->second != TileState::VISIBLE) {
            continue;
        }
        for (const auto &[layerIdentifier, symbolGroups]: symbolGroupsMap) {
            const auto &description = layerDescriptions.at(layerIdentifier);
            for (auto &symbolGroup: symbolGroups) {
                symbolGroup.syncAccess([&zoomIdentifier, &rotation, &scaleFactor, &now](auto group){
                    group->update(zoomIdentifier, rotation, scaleFactor, now);
                });
            }
        }
    }

    if (animationCoordinatorMap->isAnimating()) {
        mapInterface->invalidate();
    }
}

void Tiled2dMapVectorSourceSymbolDataManager::pregenerateRenderPasses() {
    auto mapInterface = this->mapInterface.lock();
    auto camera = mapInterface ? mapInterface->getCamera() : nullptr;
    if (!camera) {
        return ;
    }

    std::vector<std::shared_ptr<Tiled2dMapVectorLayer::TileRenderDescription>> renderDescriptions;

    for (const auto &[tile, symbolGroupsMap]: tileSymbolGroupMap) {
        const auto tileState = tileStateMap.find(tile);
        if (tileState == tileStateMap.end() || tileState->second != TileState::VISIBLE) {
            continue;
        }
        for (const auto &[layerIdentifier, symbolGroups]: symbolGroupsMap) {
            const int32_t index = layerNameIndexMap.at(layerIdentifier);

            std::vector<std::shared_ptr< ::RenderObjectInterface>> renderObjects;
            for (const auto &group: symbolGroups) {
                group.syncAccess([&renderObjects](auto group){
                    auto iconObject = group->iconInstancedObject;
                    if (iconObject) {
                        renderObjects.push_back(std::make_shared<RenderObject>(iconObject->asGraphicsObject()));
                    }
                    auto stretchIconObject = group->stretchedInstancedObject;
                    if (stretchIconObject) {
                        renderObjects.push_back(std::make_shared<RenderObject>(stretchIconObject->asGraphicsObject()));
                    }
                    auto textObject = group->textInstancedObject;
                    if (textObject) {
                        renderObjects.push_back(std::make_shared<RenderObject>(textObject->asGraphicsObject()));
                    }

                    auto boundingBoxLayerObject = group->boundingBoxLayerObject;
                    if (boundingBoxLayerObject) {
                        renderObjects.push_back(std::make_shared<RenderObject>(boundingBoxLayerObject->getPolygonObject()));
                    }
                });
            }
            renderDescriptions.push_back(std::make_shared<Tiled2dMapVectorLayer::TileRenderDescription>(Tiled2dMapVectorLayer::TileRenderDescription{index, renderObjects, nullptr, false}));
        }
    }

    vectorLayer.syncAccess([source = this->source, &renderDescriptions](const auto &layer){
        if(auto strong = layer.lock()) {
            strong->onRenderPassUpdate(source, true, renderDescriptions);
        }
    });

    mapInterface->invalidate();
}

bool Tiled2dMapVectorSourceSymbolDataManager::onClickUnconfirmed(const std::unordered_set<std::string> &layers, const Vec2F &posScreen) {
    return false;
}

bool Tiled2dMapVectorSourceSymbolDataManager::onClickConfirmed(const std::unordered_set<std::string> &layers, const Vec2F &posScreen) {
    auto lockSelfPtr = shared_from_this();
    auto mapInterface = lockSelfPtr ? lockSelfPtr->mapInterface.lock() : nullptr;
    auto camera = mapInterface ? mapInterface->getCamera() : nullptr;
    auto conversionHelper = mapInterface ? mapInterface->getCoordinateConverterHelper() : nullptr;
    auto strongSelectionDelegate = selectionDelegate.lock();
    if (!camera || !conversionHelper || !strongSelectionDelegate) {
        return false;
    }

    Coord clickCoords = camera->coordFromScreenPosition(posScreen);
    Coord clickCoordsRenderCoord = conversionHelper->convertToRenderSystem(clickCoords);

    double clickPadding = camera->mapUnitsFromPixels(16);

    OBB2D tinyClickBox(Quad2dD(Vec2D(clickCoordsRenderCoord.x - clickPadding, clickCoordsRenderCoord.y - clickPadding),
                               Vec2D(clickCoordsRenderCoord.x + clickPadding, clickCoordsRenderCoord.y - clickPadding),
                               Vec2D(clickCoordsRenderCoord.x + clickPadding, clickCoordsRenderCoord.y + clickPadding),
                               Vec2D(clickCoordsRenderCoord.x - clickPadding, clickCoordsRenderCoord.y + clickPadding)));

    for(const auto &[tile, symbolGroupsMap]: tileSymbolGroupMap) {
        const auto tileState = tileStateMap.find(tile);
        if (tileState == tileStateMap.end() || tileState->second != TileState::VISIBLE) {
            continue;
        }
        for (const auto &[layerIdentifier, symbolGroups] : symbolGroupsMap) {
            if (interactableLayers.count(layerIdentifier) == 0) {
                continue;
            }
            for (const auto &symbolGroup : symbolGroups) {
                auto result = symbolGroup.syncAccess([&tinyClickBox](auto group){
                    return group->onClickConfirmed(tinyClickBox);
                });
                if (result) {
                    strongSelectionDelegate->didSelectFeature(std::get<1>(*result), layerIdentifier, conversionHelper->convert(CoordinateSystemIdentifiers::EPSG4326(), std::get<0>(*result)));
                    return true;
                }
            }
        }
    }
    return false;

}

bool Tiled2dMapVectorSourceSymbolDataManager::onDoubleClick(const std::unordered_set<std::string> &layers, const Vec2F &posScreen) {
    return false;
}

bool Tiled2dMapVectorSourceSymbolDataManager::onLongPress(const std::unordered_set<std::string> &layers, const Vec2F &posScreen) {
    return false;
}

bool Tiled2dMapVectorSourceSymbolDataManager::onTwoFingerClick(const std::unordered_set<std::string> &layers, const Vec2F &posScreen1,
                                                          const Vec2F &posScreen2) {
    return false;
}

void Tiled2dMapVectorSourceSymbolDataManager::clearTouch() {

}

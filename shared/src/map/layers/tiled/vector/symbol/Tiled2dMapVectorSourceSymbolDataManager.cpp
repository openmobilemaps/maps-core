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
                                                                                 const std::shared_ptr<Tiled2dMapVectorStateManager> &featureStateManager,
                                                                                 const std::shared_ptr<Tiled2dMapVectorLayerSymbolDelegateInterface> &symbolDelegate,
                                                                                 bool persistingSymbolPlacement)
        : Tiled2dMapVectorSourceDataManager(vectorLayer, mapDescription, layerConfig, source, readyManager, featureStateManager),
        fontLoader(fontLoader), vectorSource(vectorSource),
        animationCoordinatorMap(std::make_shared<SymbolAnimationCoordinatorMap>()),
        symbolDelegate(symbolDelegate),
        persistingSymbolPlacement(persistingSymbolPlacement) {

    for (const auto &layer: mapDescription->layers) {
        if (layer->getType() == VectorLayerType::symbol && layer->source == source) {
            layerDescriptions.insert({layer->identifier, std::static_pointer_cast<SymbolVectorLayerDescription>(layer)});
            if (layer->isInteractable(EvaluationContext(0.0, 0.0, std::make_shared<FeatureContext>(), featureStateManager))) {
                interactableLayers.insert(layer->identifier);
            }
        }
    }
}

void Tiled2dMapVectorSourceSymbolDataManager::onAdded(const std::weak_ptr<::MapInterface> &mapInterface) {
    Tiled2dMapVectorSourceDataManager::onAdded(mapInterface);
    auto strongMapInterface = mapInterface.lock();
    if (strongMapInterface) {
        fontProviderManager = Actor<Tiled2dMapVectorSymbolFontProviderManager>(std::make_shared<Mailbox>(strongMapInterface->getScheduler()), fontLoader);
    }
    textHelper.setMapInterface(mapInterface);
}

void Tiled2dMapVectorSourceSymbolDataManager::onRemoved() {
    Tiled2dMapVectorSourceDataManager::onRemoved();
    mapInterface = std::weak_ptr<MapInterface>();
    fontProviderManager = Actor<Tiled2dMapVectorSymbolFontProviderManager>();
    // Clear (on GLThread!) and remove all symbol groups - they're dependant on the mapInterface
}

void Tiled2dMapVectorSourceSymbolDataManager::pause() {
    for (const auto &[tileInfo, tileSymbolGroups]: tileSymbolGroupMap) {
        for (const auto &[s, symbolGroups]: tileSymbolGroups) {
            for (const auto &symbolGroup: std::get<1>(symbolGroups)) {
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
            for (const auto &symbolGroup: std::get<1>(symbolGroups)) {
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
            for (const auto &symbolGroup: std::get<1>(symbolGroups)) {
                symbolGroup.message(MailboxDuplicationStrategy::replaceNewest, &Tiled2dMapVectorSymbolGroup::setAlpha, alpha);
            }
        }
    }
}

void Tiled2dMapVectorSourceSymbolDataManager::updateLayerDescriptions(std::vector<Tiled2dMapVectorLayerUpdateInformation> layerUpdates) {
    auto mapInterface = this->mapInterface.lock();
    if (!mapInterface) {
        return;
    }

    auto const &currentTileInfos = vectorSource.converse(&Tiled2dMapVectorSource::getCurrentTiles).get();

    for (const auto layerUpdate: layerUpdates) {
        if (layerUpdate.layerDescription->source != source || layerUpdate.layerDescription->getType() != VectorLayerType::symbol) {
            continue;
        }

        auto oldStyle = std::static_pointer_cast<SymbolVectorLayerDescription>(layerUpdate.oldLayerDescription);
        auto castedDescription = std::static_pointer_cast<SymbolVectorLayerDescription>(layerUpdate.layerDescription);

        bool iconWasAdded = false;

        if (oldStyle->style.hasIconImagePotentially() != castedDescription->style.hasIconImagePotentially() &&  oldStyle->style.hasIconImagePotentially() == false) {
            iconWasAdded = true;
        }

        layerDescriptions.erase(layerUpdate.layerDescription->identifier);
        layerDescriptions.insert({layerUpdate.layerDescription->identifier, castedDescription});

        if (layerUpdate.needsTileReplace || iconWasAdded) {

            std::unordered_map<Tiled2dMapTileInfo, std::vector<Actor<Tiled2dMapVectorSymbolGroup>>> toSetup;
            std::vector<Actor<Tiled2dMapVectorSymbolGroup>> toClear;

            {
                std::lock_guard<std::recursive_mutex> updateLock(updateMutex);
                for (const auto &tileData: currentTileInfos) {
                    auto tileGroup = tileSymbolGroupMap.find(tileData.tileInfo);
                    if (tileGroup == tileSymbolGroupMap.end()) {
                        continue;
                    }

                    auto tileGroupIt = tileGroup->second.find(layerUpdate.layerDescription->identifier);
                    if (tileGroupIt != tileGroup->second.end()) {
                        for (const auto &group: std::get<1>(tileGroupIt->second)) {
                            toClear.push_back(group);
                        }
                    }

                    tileGroup->second.erase(layerUpdate.layerDescription->identifier);

                    // If new source of layer is not handled by this manager, continue
                    if (layerUpdate.layerDescription->source != source) {
                        continue;
                    }

                    const auto &dataIt = tileData.layerFeatureMaps->find(layerUpdate.layerDescription->sourceLayer);

                    if (dataIt != tileData.layerFeatureMaps->end()) {
                        // there is something in this layer to display
                        const auto &newSymbolGroups = createSymbolGroups(tileData.tileInfo, layerUpdate.layerDescription->identifier,
                                                                         dataIt->second);
                        if (!newSymbolGroups.empty()) {
                            for (const auto &group: newSymbolGroups) {
                                std::get<1>(tileSymbolGroupMap.at(tileData.tileInfo)[layerUpdate.layerDescription->identifier]).push_back(group);
                                std::get<0>(tileSymbolGroupMap.at(tileData.tileInfo)[layerUpdate.layerDescription->identifier]).increaseBase();
                            }
                        }
                    }

                    this->tilesToClear = std::vector(toClear);
                    this->tileStatesToRemove = std::unordered_set<Tiled2dMapVersionedTileInfo>{};
                    this->tileStateUpdates = std::unordered_map<Tiled2dMapVersionedTileInfo, TileState>{};
                }
            }

            auto selfActor = WeakActor(mailbox, weak_from_this());
            selfActor.messagePrecisely(MailboxDuplicationStrategy::replaceNewest, MailboxExecutionEnvironment::graphics,
                                       &Tiled2dMapVectorSourceSymbolDataManager::updateSymbolGroups);
        } else {
            for (const auto &[tileInfo, groupMap]: tileSymbolGroupMap) {
                auto const groupsIt = groupMap.find(layerUpdate.layerDescription->identifier);
                if (groupsIt != groupMap.end()) {
                    for (const auto &group: std::get<1>(groupsIt->second)) {
                        group.messagePrecisely(MailboxDuplicationStrategy::replaceNewest, MailboxExecutionEnvironment::graphics,
                                               &Tiled2dMapVectorSymbolGroup::updateLayerDescription, castedDescription);
                    }
                }
            }

            mapInterface->invalidate();
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

        {
            std::lock_guard<std::recursive_mutex> updateLock(updateMutex);
            for (const auto &tileData: currentTileInfos) {
                auto tileGroup = tileSymbolGroupMap.find(tileData.tileInfo);
                if (tileGroup == tileSymbolGroupMap.end()) {
                    continue;
                }

                auto tileGroupIt = tileGroup->second.find(layerDescription->identifier);
                if (tileGroupIt != tileGroup->second.end()) {
                    for (const auto &group: std::get<1>(tileGroupIt->second)) {
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
                    const auto &newSymbolGroups = createSymbolGroups(tileData.tileInfo, layerDescription->identifier,
                                                                     dataIt->second);
                    if (!newSymbolGroups.empty()) {
                        for (const auto &group: newSymbolGroups) {
                            std::get<1>(tileSymbolGroupMap.at(tileData.tileInfo)[layerDescription->identifier]).push_back(group);
                            std::get<0>(tileSymbolGroupMap.at(tileData.tileInfo)[layerDescription->identifier]).increaseBase();
                        }
                    }
                }

                this->tilesToClear = std::vector(toClear);
                this->tileStatesToRemove = std::unordered_set<Tiled2dMapVersionedTileInfo>{};
                this->tileStateUpdates = std::unordered_map<Tiled2dMapVersionedTileInfo, TileState>{};
            }
        }

        auto selfActor = WeakActor(mailbox, weak_from_this());
        selfActor.messagePrecisely(MailboxDuplicationStrategy::replaceNewest, MailboxExecutionEnvironment::graphics,
                                   &Tiled2dMapVectorSourceSymbolDataManager::updateSymbolGroups);
    } else {
        for (const auto &[tileInfo, groupMap]: tileSymbolGroupMap) {
            auto const groupsIt = groupMap.find(layerDescription->identifier);
            if (groupsIt != groupMap.end()) {
                for (const auto &group: std::get<1>(groupsIt->second)) {
                    group.messagePrecisely(MailboxDuplicationStrategy::replaceNewest, MailboxExecutionEnvironment::graphics,
                                           &Tiled2dMapVectorSymbolGroup::updateLayerDescription, castedDescription);
                }
            }
        }

        mapInterface->invalidate();
    }
}

void Tiled2dMapVectorSourceSymbolDataManager::reloadLayerContent(const std::vector<std::tuple<std::string, std::string>> &sourceLayerIdentifierPairs) {
    auto mapInterface = this->mapInterface.lock();
    if (!mapInterface) {
        return;
    }

    auto const &currentTileInfos = vectorSource.converse(&Tiled2dMapVectorSource::getCurrentTiles).get();

    {
        std::lock_guard<std::recursive_mutex> updateLock(updateMutex);
        for (const auto &[sourceLayer, layerIdentifier]: sourceLayerIdentifierPairs) {
            for (const auto &tileData: currentTileInfos) {
                auto tileGroup = tileSymbolGroupMap.find(tileData.tileInfo);
                if (tileGroup == tileSymbolGroupMap.end()) {
                    continue;
                }

                auto tileGroupIt = tileGroup->second.find(layerIdentifier);
                if (tileGroupIt != tileGroup->second.end()) {
                    for (const auto &group: std::get<1>(tileGroupIt->second)) {
                        this->tilesToClear.push_back(group);
                    }
                }

                tileGroup->second.erase(layerIdentifier);

                const auto &dataIt = tileData.layerFeatureMaps->find(sourceLayer);

                if (dataIt != tileData.layerFeatureMaps->end()) {
                    // there is something in this layer to display
                    const auto &newSymbolGroups = createSymbolGroups(tileData.tileInfo, layerIdentifier,
                                                                     dataIt->second);
                    if (!newSymbolGroups.empty()) {
                        for (const auto &group: newSymbolGroups) {
                            std::get<1>(tileSymbolGroupMap.at(tileData.tileInfo)[layerIdentifier]).push_back(group);
                            std::get<0>(tileSymbolGroupMap.at(tileData.tileInfo)[layerIdentifier]).increaseBase();
                        }
                    }
                }
            }
        }
    }

    auto selfActor = WeakActor(mailbox, weak_from_this());
    selfActor.messagePrecisely(MailboxDuplicationStrategy::replaceNewest, MailboxExecutionEnvironment::graphics,
                               &Tiled2dMapVectorSourceSymbolDataManager::updateSymbolGroups);

    mapInterface->invalidate();

}

void Tiled2dMapVectorSourceSymbolDataManager::onVectorTilesUpdated(const std::string &sourceName,
                                                                   std::unordered_set<Tiled2dMapVectorTileInfo> currentTileInfos) {
    if (updateFlag.test_and_set()) {
        return;
    }

    auto mapInterface = this->mapInterface.lock();
    auto graphicsFactory = mapInterface ? mapInterface->getGraphicsObjectFactory() : nullptr;
    auto coordinateConverterHelper = mapInterface ? mapInterface->getCoordinateConverterHelper() : nullptr;
    auto shaderFactory = mapInterface ? mapInterface->getShaderFactory() : nullptr;
    if (!graphicsFactory || !shaderFactory) {
        updateFlag.clear();
        return;
    }

    // Just insert pointers here since we will only access the objects inside this method where we know that currentTileInfos is retained
    std::vector<const Tiled2dMapVectorTileInfo *> tilesToAdd;
    std::vector<Actor<Tiled2dMapVectorSymbolGroup>> toClear;
    std::unordered_set<Tiled2dMapVersionedTileInfo> toRemove;
    std::unordered_map<Tiled2dMapVersionedTileInfo, TileState> tileStateUpdates;

    {
        std::lock_guard<std::recursive_mutex> updateLock(updateMutex);
        updateFlag.clear();

        for (const auto &vectorTileInfo: currentTileInfos) {
            if (tileSymbolGroupMap.count(vectorTileInfo.tileInfo) == 0) {
                tilesToAdd.push_back(&vectorTileInfo);
            } else {
                auto tileStateIt = tileStateMap.find(vectorTileInfo.tileInfo);
                if (tileStateIt == tileStateMap.end() || (tileStateIt->second != vectorTileInfo.state)) {
                    tileStateUpdates[vectorTileInfo.tileInfo] = vectorTileInfo.state;
                }
                if (vectorTileInfo.state == TileState::IN_SETUP) {
                    // Tile has been cleared in source, but is still available here
                    auto tileIt = tileSymbolGroupMap.find(vectorTileInfo.tileInfo);
                    if (tileIt != tileSymbolGroupMap.end()) {
                        bool fullyReady = true;
                        for (auto &[layerIdentifier, counterGroupTuple]: tileIt->second) {
                            fullyReady &= (std::get<0>(counterGroupTuple).isDone());
                        }
                        if (fullyReady) {
                            // Tile is ready - propagate to source
                            readyManager.message(&Tiled2dMapVectorReadyManager::didProcessData, readyManagerIndex, vectorTileInfo.tileInfo, 0);
                        }
                    }
                }
            }
        }

        for (const auto &[tileInfo, groupMap]: tileSymbolGroupMap) {
            bool found = false;
            for (const auto &currentTile: currentTileInfos) {
                if (tileInfo == currentTile.tileInfo) {
                    found = true;
                    break;
                }
            }
            if (!found) {
                toRemove.insert(tileInfo);
                for (const auto &[_, groups]: groupMap) {
                    for (const auto &group: std::get<1>(groups)) {
                        toClear.push_back(group);
                    }
                }
            }
        }

        if (tilesToAdd.empty() && toRemove.empty() && tileStateUpdates.empty()) {
            return;
        }

        std::unordered_map<Tiled2dMapTileInfo, std::vector<Actor<Tiled2dMapVectorSymbolGroup>>> toSetup;

        for (const auto &tile: tilesToAdd) {
            tileSymbolGroupMap[tile->tileInfo] = {};
            tileStateUpdates[tile->tileInfo] = tile->state;
            size_t notReadyCount = 0;

            for (const auto &[layerIdentifier, layer]: layerDescriptions) {
                const auto &dataIt = tile->layerFeatureMaps->find(layer->sourceLayer);
                if (dataIt != tile->layerFeatureMaps->end()) {
                    // there is something in this layer to display
                    const auto &newSymbolGroups = createSymbolGroups(tile->tileInfo, layerIdentifier, dataIt->second);
                    if (!newSymbolGroups.empty()) {
                        for (const auto &group: newSymbolGroups) {
                            std::get<1>(tileSymbolGroupMap.at(tile->tileInfo)[layerIdentifier]).push_back(group);
                            std::get<0>(tileSymbolGroupMap.at(tile->tileInfo)[layerIdentifier]).increaseBase();
                            notReadyCount += 1;
                        }
                    }
                }
            }

            readyManager.message(&Tiled2dMapVectorReadyManager::didProcessData, readyManagerIndex, tile->tileInfo, notReadyCount);
        }

        this->tilesToClear = std::vector(toClear);
        this->tileStatesToRemove = std::unordered_set<Tiled2dMapVersionedTileInfo>(toRemove);
        this->tileStateUpdates = std::unordered_map<Tiled2dMapVersionedTileInfo, TileState>(tileStateUpdates);
    }

    auto selfActor = WeakActor(mailbox, weak_from_this());
    selfActor.messagePrecisely(MailboxDuplicationStrategy::replaceNewest, MailboxExecutionEnvironment::graphics,
                               &Tiled2dMapVectorSourceSymbolDataManager::updateSymbolGroups);
    vectorLayer.message(&Tiled2dMapVectorLayer::invalidateCollisionState);
}

std::vector<Actor<Tiled2dMapVectorSymbolGroup>> Tiled2dMapVectorSourceSymbolDataManager::createSymbolGroups(const Tiled2dMapVersionedTileInfo &tileInfo,
                                                                                                            const std::string &layerIdentifier,
                                                                                                            std::shared_ptr<std::vector<Tiled2dMapVectorTileInfo::FeatureTuple>> features) {
    auto selfActor = WeakActor(mailbox, weak_from_this());
    std::vector<Actor<Tiled2dMapVectorSymbolGroup>> symbolGroups = {};
    uint32_t numFeatures = features ? (uint32_t) features->size() : 0;
    for (uint32_t featuresBase = 0; featuresBase < numFeatures; featuresBase += maxNumFeaturesPerGroup) {
        auto mailbox = std::make_shared<Mailbox>(mapInterface.lock()->getScheduler());
        Actor<Tiled2dMapVectorSymbolGroup> symbolGroupActor = Actor<Tiled2dMapVectorSymbolGroup>(mailbox,
                                                                                                 featuresBase, // unique within tile
                                                                                                 mapInterface,
                                                                                                 layerConfig,
                                                                                                 fontProviderManager.weakActor<Tiled2dMapVectorFontProvider>(),
                                                                                                 tileInfo,
                                                                                                 layerIdentifier,
                                                                                                 layerDescriptions.at(
                                                                                                         layerIdentifier),
                                                                                                 featureStateManager,
                                                                                                 symbolDelegate,
                                                                                                 persistingSymbolPlacement);
        symbolGroupActor.message(&Tiled2dMapVectorSymbolGroup::initialize, features, featuresBase,
                                 std::min(featuresBase + maxNumFeaturesPerGroup, numFeatures) - featuresBase,
                                 animationCoordinatorMap, selfActor, alpha);
        symbolGroups.push_back(symbolGroupActor);
    }
    return symbolGroups;
}

void Tiled2dMapVectorSourceSymbolDataManager::onSymbolGroupInitialized(bool success, const Tiled2dMapVersionedTileInfo &tileInfo,
                                                                       const std::string &layerIdentifier,
                                                                       const WeakActor<Tiled2dMapVectorSymbolGroup> &symbolGroup) {
    auto weakGroup = symbolGroup.unsafe();
    auto group = weakGroup.lock();
    if (group) {
        uint32_t targetGroupId = group->groupId;
        auto tileIt = tileSymbolGroupMap.find(tileInfo);
        if (tileIt != tileSymbolGroupMap.end()) {
            auto layerIt = tileIt->second.find(layerIdentifier);
            bool isLast = std::get<0>(layerIt->second).decreaseAndCheckFinal();
            if (!success) {
                if (layerIt != tileIt->second.end()) {
                    for (auto groupIt = std::get<1>(layerIt->second).begin(); groupIt != std::get<1>(layerIt->second).end(); groupIt++) {
                        if (groupIt->unsafe()->groupId == targetGroupId) {
                            std::get<1>(layerIt->second).erase(groupIt);
                            break;
                        }
                    }
                }
            }

            if (isLast) {
                auto selfActor = WeakActor(mailbox, weak_from_this());
                selfActor.message(MailboxExecutionEnvironment::graphics, &Tiled2dMapVectorSourceSymbolDataManager::setupSymbolGroups, tileInfo, layerIdentifier);
            }
        }
    }
}

void Tiled2dMapVectorSourceSymbolDataManager::setupSymbolGroups(const Tiled2dMapVersionedTileInfo &tileInfo, const std::string &layerIdentifier) {
    auto tileIt = tileSymbolGroupMap.find(tileInfo);
    if (tileIt == tileSymbolGroupMap.end()) {
        return;
    }
    auto layerIt = tileIt->second.find(layerIdentifier);
    if (layerIt == tileIt->second.end()) {
        return;
    }

    for (const auto &symbolGroup: std::get<1>(layerIt->second)) {
        symbolGroup.syncAccess([&](auto group){
            group->setupObjects(spriteData, spriteTexture);
        });
    }
    vectorLayer.message(&Tiled2dMapVectorLayer::invalidateCollisionState);
    readyManager.message(&Tiled2dMapVectorReadyManager::setReady, readyManagerIndex, tileInfo, std::get<0>(layerIt->second).baseValue);
}

void Tiled2dMapVectorSourceSymbolDataManager::updateSymbolGroups() {
    auto mapInterface = this->mapInterface.lock();
    auto context = mapInterface ? mapInterface->getRenderingContext() : nullptr;
    if (!context) { return; }

    {
        std::lock_guard<std::recursive_mutex> updateLock(updateMutex);
        for (const auto &[tile, state]: tileStateUpdates) {
            auto tileStateIt = tileStateMap.find(tile);
            if (tileStateIt == tileStateMap.end()) {
                tileStateMap[tile] = state;
            } else {
                if (tileStateIt->second == TileState::CACHED && state != TileState::CACHED) {
                    auto tileSymbolGroupMapIt = tileSymbolGroupMap.find(tile);
                    if (tileSymbolGroupMapIt != tileSymbolGroupMap.end()) {
                        for (const auto &[layerIdentifier, symbolGroups]: tileSymbolGroupMapIt->second) {
                            for (auto &symbolGroup: std::get<1>(symbolGroups)) {
                                symbolGroup.syncAccess([&](auto group) {
                                    group->removeFromCache();
                                });
                            }
                        }
                    }
                }
                tileStateIt->second = state;
            }
            if (state == TileState::CACHED) {
                auto tileSymbolGroupMapIt = tileSymbolGroupMap.find(tile);
                if (tileSymbolGroupMapIt != tileSymbolGroupMap.end()) {
                    for (const auto &[layerIdentifier, symbolGroups]: tileSymbolGroupMapIt->second) {
                        for (auto &symbolGroup: std::get<1>(symbolGroups)) {
                            symbolGroup.syncAccess([&](auto group) {
                                group->placedInCache();
                            });
                        }
                    }
                }
            }
        }
        tileStateUpdates.clear();

        for (const auto &tile: tileStatesToRemove) {
            tileStateMap.erase(tile);
            tileSymbolGroupMap.erase(tile);
        }

        std::unordered_set<Tiled2dMapVersionedTileInfo> localToRemove = std::unordered_set(tileStatesToRemove);
        tileStatesToRemove.clear();
        readyManager.syncAccess([localToRemove](auto manager) {
            manager->remove(localToRemove);
        });

        bool clearCoordinator = !tilesToClear.empty();
        for (const auto &symbolGroup: tilesToClear) {
            symbolGroup.syncAccess([](auto symbolGroup) {
                symbolGroup->clear();
            });
        }
        tilesToClear.clear();

        if (clearCoordinator) {
            animationCoordinatorMap->clearAnimationCoordinators();
        }
    }

    pregenerateRenderPasses();
    vectorLayer.message(&Tiled2dMapVectorLayer::invalidateCollisionState);
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
            for (auto &symbolGroup: std::get<1>(symbolGroups)) {
                symbolGroup.message(MailboxExecutionEnvironment::graphics, &Tiled2dMapVectorSymbolGroup::setupObjects, spriteData, spriteTexture, std::nullopt);
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
        std::vector<SymbolObjectCollisionWrapper> allObjects;

        for (const auto &[tile, symbolGroupsMap]: tileSymbolGroupMap) {
            const auto tileState = tileStateMap.find(tile);
            if (tileState == tileStateMap.end() || tileState->second != TileState::VISIBLE) {
                continue;
            }
            const auto objectsIt = symbolGroupsMap.find(layerIdentifier);
            if (objectsIt != symbolGroupsMap.end()) {
                for (auto &symbolGroup: std::get<1>(objectsIt->second)) {
                    symbolGroup.syncAccess([&allObjects](auto group){
                        for(auto& o : group->getSymbolObjectsForCollision()) {
                            allObjects.push_back(o);
                        }
                    });
                }
            }
        }
        
        std::stable_sort(allObjects.rbegin(), allObjects.rend());

        for (const auto &objectWrapper: allObjects) {
            objectWrapper.symbolObject->collisionDetection(zoomIdentifier, rotation, scaleFactor, collisionGrid);
        }
    }
}

bool Tiled2dMapVectorSourceSymbolDataManager::update(long long now) {
    auto mapInterface = this->mapInterface.lock();
    auto camera = mapInterface ? mapInterface->getCamera() : nullptr;
    auto renderingContext = mapInterface ? mapInterface->getRenderingContext() : nullptr;
    auto converter = mapInterface ? mapInterface->getCoordinateConverterHelper() : nullptr;
    if (!camera || !converter || !renderingContext) {
        return false;
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
            for (auto &symbolGroup: std::get<1>(symbolGroups)) {
                symbolGroup.syncAccess([&zoomIdentifier, &rotation, &scaleFactor, &now](auto group){
                    group->update(zoomIdentifier, rotation, scaleFactor, now);
                });
            }
        }
    }

    if (animationCoordinatorMap->isAnimating()) {
        mapInterface->invalidate();
        return true;
    }

    return false;
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
            bool selfMasked = selfMaskedLayers.find(index) != selfMaskedLayers.end();

            std::vector<std::shared_ptr< ::RenderObjectInterface>> renderObjects;
            for (const auto &group: std::get<1>(symbolGroups)) {
                group.syncAccess([&renderObjects](auto group){
                    auto groupRenderObjects = group->getRenderObjects();
                    renderObjects.insert(renderObjects.end(), std::make_move_iterator(groupRenderObjects.begin()), std::make_move_iterator(groupRenderObjects.end()));
                });
            }

            const auto optRenderPassIndex = mapDescription->layers[index]->renderPassIndex;
            const int32_t renderPassIndex = optRenderPassIndex ? *optRenderPassIndex : 0;
            renderDescriptions.push_back(std::make_shared<Tiled2dMapVectorLayer::TileRenderDescription>(Tiled2dMapVectorLayer::TileRenderDescription{index, renderObjects, nullptr, false, selfMasked, renderPassIndex}));
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
    if (!camera || !conversionHelper) {
        return false;
    }

    Coord clickCoords = camera->coordFromScreenPosition(posScreen);

    return performClick(layers, clickCoords);
}

bool Tiled2dMapVectorSourceSymbolDataManager::performClick(const std::unordered_set<std::string> &layers, const Coord &coord) {
    auto lockSelfPtr = shared_from_this();
    auto mapInterface = lockSelfPtr ? lockSelfPtr->mapInterface.lock() : nullptr;
    auto camera = mapInterface ? mapInterface->getCamera() : nullptr;
    auto conversionHelper = mapInterface ? mapInterface->getCoordinateConverterHelper() : nullptr;
    auto strongSelectionDelegate = selectionDelegate.lock();
    if (!camera || !conversionHelper || !strongSelectionDelegate) {
        return false;
    }

    Coord clickCoordsRenderCoord = conversionHelper->convertToRenderSystem(coord);

    double clickPadding = camera->mapUnitsFromPixels(16);
    CircleD clickHitCircle(clickCoordsRenderCoord.x, clickCoordsRenderCoord.y, clickPadding);

    for(const auto &[tile, symbolGroupsMap]: tileSymbolGroupMap) {
        const auto tileState = tileStateMap.find(tile);
        if (tileState == tileStateMap.end() || tileState->second != TileState::VISIBLE) {
            continue;
        }
        for (const auto &[layerIdentifier, symbolGroups] : symbolGroupsMap) {
            if (interactableLayers.find(layerIdentifier) == interactableLayers.end() || layers.find(layerIdentifier) == layers.end()) {
                continue;
            }
            for (const auto &symbolGroup : std::get<1>(symbolGroups)) {
                auto result = symbolGroup.syncAccess([&clickHitCircle](auto group){
                    return group->onClickConfirmed(clickHitCircle);
                });
                if (result) {
                    if (strongSelectionDelegate->didSelectFeature(std::get<1>(*result), layerIdentifier, conversionHelper->convert(CoordinateSystemIdentifiers::EPSG4326(), std::get<0>(*result)))) {
                        return true;
                    }
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

void Tiled2dMapVectorSourceSymbolDataManager::setSymbolDelegate(const /*not-null*/ std::shared_ptr<Tiled2dMapVectorLayerSymbolDelegateInterface> & symbolDelegate) {
    this->symbolDelegate = symbolDelegate;
}

void Tiled2dMapVectorSourceSymbolDataManager::enableAnimations(bool enabled) {
    animationCoordinatorMap->enableAnimations(enabled);
}

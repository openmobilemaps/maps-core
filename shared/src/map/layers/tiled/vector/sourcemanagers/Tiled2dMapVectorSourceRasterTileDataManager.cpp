/*
 * Copyright (c) 2021 Ubique Innovation AG <https://www.ubique.ch>
 *
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this
 *  file, You can obtain one at https://mozilla.org/MPL/2.0/.
 *
 *  SPDX-License-Identifier: MPL-2.0
 */

#include "Tiled2dMapVectorSourceRasterTileDataManager.h"
#include "PolygonCompare.h"
#include "Tiled2dMapVectorLayer.h"

Tiled2dMapVectorSourceRasterTileDataManager::Tiled2dMapVectorSourceRasterTileDataManager(const WeakActor<Tiled2dMapVectorLayer> &vectorLayer,
                                                                                         const std::shared_ptr<VectorMapDescription> &mapDescription,
                                                                                         const std::shared_ptr<Tiled2dMapVectorLayerConfig> &layerConfig,
                                                                                         const std::shared_ptr<VectorLayerDescription> &layerDescription,
                                                                                         const WeakActor<Tiled2dMapRasterSource> &rasterSource,
                                                                                         const Actor<Tiled2dMapVectorReadyManager> &readyManager,
                                                                                         const std::shared_ptr<Tiled2dMapVectorStateManager> &featureStateManager,
                                                                                         double dpFactor)
: Tiled2dMapVectorSourceTileDataManager(vectorLayer, mapDescription, layerConfig, layerDescription->source, readyManager, featureStateManager),
layerDescription(layerDescription), rasterSource(rasterSource), dpFactor(dpFactor) {}

void Tiled2dMapVectorSourceRasterTileDataManager::onRasterTilesUpdated(const std::string &layerName,
                                                                       std::unordered_set<Tiled2dMapRasterTileInfo> currentTileInfos) {
    if (updateFlag.test_and_set()) {
        return;
    }

    auto mapInterface = this->mapInterface.lock();
    {
        auto graphicsFactory = mapInterface ? mapInterface->getGraphicsObjectFactory() : nullptr;
        auto coordinateConverterHelper = mapInterface ? mapInterface->getCoordinateConverterHelper() : nullptr;
        auto shaderFactory = mapInterface ? mapInterface->getShaderFactory() : nullptr;
        if (!graphicsFactory || !shaderFactory) {
            updateFlag.clear();
            return;
        }

        std::unordered_set<Tiled2dMapRasterTileInfo> tilesToAdd;
        std::unordered_set<Tiled2dMapRasterTileInfo> tilesToKeep;
        std::unordered_set<Tiled2dMapVersionedTileInfo> tilesToRemove;
        std::unordered_map<Tiled2dMapVersionedTileInfo, TileState> tileStateUpdates;
        std::unordered_map<Tiled2dMapVersionedTileInfo, Tiled2dMapLayerMaskWrapper> newTileMasks;

        {
            std::lock_guard<std::recursive_mutex> updateLock(updateMutex);
            updateFlag.clear();

            for (const auto &rasterTileInfo: currentTileInfos) {
                if (tiles.count(rasterTileInfo.tileInfo) == 0) {
                    tilesToAdd.insert(rasterTileInfo);
                } else {
                    tilesToKeep.insert(rasterTileInfo);
                    if (rasterTileInfo.state == TileState::IN_SETUP) {
                        // Tile has been cleared in source, but is still available here
                        if (tilesReadyControlSet.find(rasterTileInfo.tileInfo) == tilesReadyControlSet.end()) {
                            // Tile is ready - propagate to source
                            readyManager.message(&Tiled2dMapVectorReadyManager::didProcessData, readyManagerIndex,
                                                 rasterTileInfo.tileInfo, 0);
                        }
                    }
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

            for (const auto &tileEntry: tilesToKeep) {
                tileStateUpdates[tileEntry.tileInfo] = tileEntry.state;

                size_t existingPolygonHash = 0;
                auto it = tileMaskMap.find(tileEntry.tileInfo);
                if (it != tileMaskMap.end()) {
                    existingPolygonHash = it->second.getPolygonHash();
                }
                const size_t hash = std::hash<std::vector<::PolygonCoord>>()(tileEntry.masks);

                if (hash != existingPolygonHash) {

                    const auto &tileMask = std::make_shared<PolygonMaskObject>(graphicsFactory,
                                                                               coordinateConverterHelper);

                    tileMask->setPolygons(tileEntry.masks);

                    newTileMasks[tileEntry.tileInfo] = Tiled2dMapLayerMaskWrapper(tileMask, hash);
                }
            }

            if (tilesToAdd.empty() && tilesToRemove.empty() && newTileMasks.empty() && tileStateUpdates.empty()) return;

            for (const auto &tile: tilesToAdd) {

                std::unordered_set<int32_t> indexControlSet;

                tiles[tile.tileInfo] = {};
                tileStateUpdates[tile.tileInfo] = tile.state;

                for (int32_t index = 0; index < mapDescription->layers.size(); index++) {
                    auto const &layer = mapDescription->layers.at(index);
                    if (layer->getType() != VectorLayerType::raster || layer->source != layerName) {
                        continue;
                    }

                    if (!(layer->minZoom <= tile.tileInfo.tileInfo.zoomIdentifier && layer->maxZoom >= tile.tileInfo.tileInfo.zoomIdentifier)) {
                        continue;
                    }

                    EvaluationContext evalContext = EvaluationContext(tile.tileInfo.tileInfo.zoomIdentifier, dpFactor,
                                                                      std::make_shared<FeatureContext>(), featureStateManager);
                    if (layerDescription->filter != nullptr && !layerDescription->filter->evaluateOr(evalContext, false)) {
                        continue;
                    }

                    const auto data = tile.textureHolder;
                    if (data) {

                        std::string identifier = layer->identifier;
                        Actor<Tiled2dMapVectorTile> actor = createTileActor(tile.tileInfo, layer);

                        if (actor) {
                            if (auto strongSelectionDelegate = selectionDelegate.lock()) {
                                actor.message(&Tiled2dMapVectorTile::setSelectionDelegate, strongSelectionDelegate);
                            }

                            indexControlSet.insert(index);

                            tiles[tile.tileInfo].push_back({index, identifier, actor.strongActor<Tiled2dMapVectorTile>()});

                            actor.message(&Tiled2dMapVectorTile::setRasterTileData, data);
                        }
                    }
                }

                if (!indexControlSet.empty()) {
                    tilesReadyControlSet[tile.tileInfo] = indexControlSet;
                }

                readyManager.message(&Tiled2dMapVectorReadyManager::didProcessData, readyManagerIndex, tile.tileInfo, indexControlSet.empty() ? 0 : 1);
            }

            this->tileMasksToSetup = std::unordered_map<Tiled2dMapVersionedTileInfo, Tiled2dMapLayerMaskWrapper>(newTileMasks);
            this->tilesToRemove = std::unordered_set<Tiled2dMapVersionedTileInfo>(tilesToRemove);
            this->tileStateUpdates = std::unordered_map<Tiled2dMapVersionedTileInfo, TileState>(tileStateUpdates);
        }

        auto castedMe = std::static_pointer_cast<Tiled2dMapVectorSourceTileDataManager>(shared_from_this());
        auto selfActor = WeakActor<Tiled2dMapVectorSourceTileDataManager>(mailbox, castedMe);
        noPendingUpdateMasks.clear();
    }
    mapInterface->invalidate();

}

void Tiled2dMapVectorSourceRasterTileDataManager::onTileCompletelyReady(const Tiled2dMapVersionedTileInfo tileInfo) {
    readyManager.message(&Tiled2dMapVectorReadyManager::setReady, readyManagerIndex, tileInfo, 1);
}

void Tiled2dMapVectorSourceRasterTileDataManager::updateLayerDescription(std::shared_ptr<VectorLayerDescription> layerDescription,
                                                                     int32_t legacyIndex, bool needsTileReplace) {
    auto mapInterface = this->mapInterface.lock();
    if (!mapInterface) {
        return;
    }

    auto const &currentTileInfos = rasterSource.converse(&Tiled2dMapRasterSource::getCurrentTiles).get();

    std::unordered_set<Tiled2dMapTileInfo> currentTilesStrippedInfos;
    for (const auto &tile : currentTileInfos) {
        currentTilesStrippedInfos.insert(tile.tileInfo.tileInfo);
    }

    for (const auto &tileData: currentTileInfos) {
        auto subTiles = tiles.find(tileData.tileInfo);
        if (subTiles == tiles.end()) {
            continue;
        }

        std::vector<Actor<Tiled2dMapVectorTile>> tilesToClear;

        EvaluationContext evalContext = EvaluationContext(subTiles->first.tileInfo.zoomIdentifier, dpFactor,
                                                          std::make_shared<FeatureContext>(), featureStateManager);
        if (currentTilesStrippedInfos.find(subTiles->first.tileInfo) == currentTilesStrippedInfos.end() ||
            (layerDescription->filter != nullptr && !layerDescription->filter->evaluateOr(evalContext, false))) {
            for (const auto &[position, id, tile] : subTiles->second) {
                tilesToClear.push_back(tile);
            }
            subTiles->second.clear();
        } else if (needsTileReplace) {
            // Remove invalid legacy tile (only one - identifier is unique)
            auto legacyPos = std::find_if(subTiles->second.begin(), subTiles->second.end(),
                                            [&identifier = layerDescription->identifier]
                                                    (const std::tuple<int32_t, std::string, Actor<Tiled2dMapVectorTile>> &subTile) {
                                                return std::get<1>(subTile) == identifier;
                                            });
            if (legacyPos != subTiles->second.end()) {
                tilesToClear.push_back(std::get<2>(*legacyPos));
                subTiles->second.erase(legacyPos);
            }

            // If new source of layer is not handled by this manager, continue
            if (layerDescription->source != source) {
                continue;
            }

            // Re-evaluate criteria for the tile creation of this specific sublayer
            if (!(layerDescription->minZoom <= tileData.tileInfo.tileInfo.zoomIdentifier && layerDescription->maxZoom >= tileData.tileInfo.tileInfo.zoomIdentifier)) {
                continue;
            }

            Actor<Tiled2dMapVectorTile> actor = createTileActor(tileData.tileInfo, layerDescription);
            if (actor) {
                if (auto strongSelectionDelegate = selectionDelegate.lock()) {
                    actor.message(&Tiled2dMapVectorTile::setSelectionDelegate, strongSelectionDelegate);
                }

                if (subTiles->second.empty()) {
                    subTiles->second.push_back(
                            {legacyIndex, layerDescription->identifier, actor.strongActor<Tiled2dMapVectorTile>()});
                } else {
                    auto insertionPoint = std::lower_bound(subTiles->second.begin(), subTiles->second.end(), legacyIndex, [](const auto& subTile, int index) {
                        return std::get<0>(subTile) < index;
                    });

                    subTiles->second.insert(insertionPoint, {legacyIndex, layerDescription->identifier, actor.strongActor<Tiled2dMapVectorTile>()});
                }

                auto indexControlSet = tilesReadyControlSet.find(tileData.tileInfo);
                if (indexControlSet == tilesReadyControlSet.end()) {
                    tilesReadyControlSet[tileData.tileInfo] = {legacyIndex};
                } else {
                    indexControlSet->second.insert(legacyIndex);
                }
                tilesReady.erase(tileData.tileInfo);

                actor.message(&Tiled2dMapVectorTile::setRasterTileData, tileData.textureHolder);
            }
        } else {
            for (auto &[index, identifier, tile]  : subTiles->second) {

                auto indexControlSet = tilesReadyControlSet.find(tileData.tileInfo);
                if (indexControlSet == tilesReadyControlSet.end()) {
                    tilesReadyControlSet[tileData.tileInfo] = {legacyIndex};
                } else {
                    indexControlSet->second.insert(legacyIndex);
                }
                tilesReady.erase(tileData.tileInfo);

                if (identifier == layerDescription->identifier) {
                    tile.message(&Tiled2dMapVectorTile::updateRasterLayerDescription, layerDescription, tileData.textureHolder);
                    break;
                }
            }
        }

        if (!tilesToClear.empty()) {
            auto castedMe = std::static_pointer_cast<Tiled2dMapVectorSourceRasterTileDataManager>(shared_from_this());
            auto selfActor = WeakActor<Tiled2dMapVectorSourceRasterTileDataManager>(mailbox, castedMe);
            selfActor.message(MailboxExecutionEnvironment::graphics, &Tiled2dMapVectorSourceRasterTileDataManager::clearTiles, tilesToClear);
        }
    }
}

void Tiled2dMapVectorSourceRasterTileDataManager::reloadLayerContent(
        const std::vector<std::tuple<std::shared_ptr<VectorLayerDescription>, int32_t>> &descriptionLayerIndexPairs) {
    auto mapInterface = this->mapInterface.lock();
    if (!mapInterface) {
        return;
    }

    auto const &currentTileInfos = rasterSource.converse(&Tiled2dMapRasterSource::getCurrentTiles).get();

    std::unordered_set<Tiled2dMapTileInfo> currentTilesStrippedInfos;
    for (const auto &tile : currentTileInfos) {
        currentTilesStrippedInfos.insert(tile.tileInfo.tileInfo);
    }

    for (const auto &[layerDescription, layerIndex]: descriptionLayerIndexPairs) {
        for (const auto &tileData: currentTileInfos) {
            auto subTiles = tiles.find(tileData.tileInfo);
            if (subTiles == tiles.end()) {
                continue;
            }

            std::vector<Actor<Tiled2dMapVectorTile>> tilesToClear;

            EvaluationContext evalContext = EvaluationContext(subTiles->first.tileInfo.zoomIdentifier, dpFactor,
                                                              std::make_shared<FeatureContext>(), featureStateManager);
            if (currentTilesStrippedInfos.find(subTiles->first.tileInfo) == currentTilesStrippedInfos.end() ||
                (layerDescription->filter != nullptr && !layerDescription->filter->evaluateOr(evalContext, false))) {
                for (const auto &[position, id, tile] : subTiles->second) {
                    tilesToClear.push_back(tile);
                }
                subTiles->second.clear();
            } else {
                // Remove invalid legacy tile (only one - identifier is unique)
                auto legacyPos = std::find_if(subTiles->second.begin(), subTiles->second.end(),
                                              [&identifier = layerDescription->identifier]
                                                      (const std::tuple<int32_t, std::string, Actor<Tiled2dMapVectorTile>> &subTile) {
                                                  return std::get<1>(subTile) == identifier;
                                              });
                if (legacyPos != subTiles->second.end()) {
                    tilesToClear.push_back(std::get<2>(*legacyPos));
                    subTiles->second.erase(legacyPos);
                }

                Actor<Tiled2dMapVectorTile> actor = createTileActor(tileData.tileInfo, layerDescription);
                if (actor) {
                    if (auto strongSelectionDelegate = selectionDelegate.lock()) {
                        actor.message(&Tiled2dMapVectorTile::setSelectionDelegate, strongSelectionDelegate);
                    }

                    if (subTiles->second.empty()) {
                        subTiles->second.push_back(
                                {layerIndex, layerDescription->identifier, actor.strongActor<Tiled2dMapVectorTile>()});
                    } else {
                        auto insertionPoint = std::lower_bound(subTiles->second.begin(), subTiles->second.end(), layerIndex,
                                                               [](const auto &subTile, int index) {
                                                                   return std::get<0>(subTile) < index;
                                                               });

                        subTiles->second.insert(insertionPoint,
                                                {layerIndex, layerDescription->identifier,
                                                 actor.strongActor<Tiled2dMapVectorTile>()});
                    }

                    auto indexControlSet = tilesReadyControlSet.find(tileData.tileInfo);
                    if (indexControlSet == tilesReadyControlSet.end()) {
                        tilesReadyControlSet[tileData.tileInfo] = {layerIndex};
                    } else {
                        indexControlSet->second.insert(layerIndex);
                    }
                    tilesReady.erase(tileData.tileInfo);

                    actor.message(&Tiled2dMapVectorTile::setRasterTileData, tileData.textureHolder);
                }
            }

            if (!tilesToClear.empty()) {
                auto castedMe = std::static_pointer_cast<Tiled2dMapVectorSourceRasterTileDataManager>(shared_from_this());
                auto selfActor = WeakActor<Tiled2dMapVectorSourceRasterTileDataManager>(mailbox, castedMe);
                selfActor.message(MailboxExecutionEnvironment::graphics,
                                  &Tiled2dMapVectorSourceRasterTileDataManager::clearTiles,
                                  tilesToClear);
            }
        }
    }
}


void Tiled2dMapVectorSourceRasterTileDataManager::clearTiles(const std::vector<Actor<Tiled2dMapVectorTile>> &tilesToClear) {
    for (const auto &tile: tilesToClear) {
        tile.syncAccess([&](auto tileActor){
            tileActor->clear();
        });
    }
}

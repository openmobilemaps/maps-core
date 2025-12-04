/*
 * Copyright (c) 2021 Ubique Innovation AG <https://www.ubique.ch>
 *
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this
 *  file, You can obtain one at https://mozilla.org/MPL/2.0/.
 *
 *  SPDX-License-Identifier: MPL-2.0
 */

#include "Tiled2dMapVectorSourceVectorTileDataManager.h"
#include "PolygonCompare.h"
#include "Tiled2dMapVectorLayer.h"
#include "Tiled2dMapVectorLayerConstants.h"
#include "Tiled2dMapVectorTile.h"

Tiled2dMapVectorSourceVectorTileDataManager::Tiled2dMapVectorSourceVectorTileDataManager(
                                                                                         const WeakActor<Tiled2dMapVectorLayer> &vectorLayer,
                                                                                         const std::shared_ptr<VectorMapDescription> &mapDescription,
                                                                                         const std::shared_ptr<Tiled2dMapVectorLayerConfig> &layerConfig,
                                                                                         const std::string &source,
                                                                                         const WeakActor<Tiled2dMapVectorSource> &vectorSource,
                                                                                         const Actor<Tiled2dMapVectorReadyManager> &readyManager,
                                                                                         const std::shared_ptr<Tiled2dMapVectorStateManager> &featureStateManager)
: Tiled2dMapVectorSourceTileDataManager(vectorLayer, mapDescription, layerConfig, source, readyManager, featureStateManager),
vectorSource(vectorSource) {}

void Tiled2dMapVectorSourceVectorTileDataManager::onVectorTilesUpdated(const std::string &sourceName,
                                                                       VectorSet<Tiled2dMapVectorTileInfo> currentTileInfos) {
    if (updateFlag.test_and_set()) {
        return;
    }

    latestTileInfos = std::move(currentTileInfos);

    auto mapInterface = this->mapInterface.lock();
    {
        auto graphicsFactory = mapInterface ? mapInterface->getGraphicsObjectFactory() : nullptr;
        auto coordinateConverterHelper = mapInterface ? mapInterface->getCoordinateConverterHelper() : nullptr;
        auto shaderFactory = mapInterface ? mapInterface->getShaderFactory() : nullptr;
        if (!graphicsFactory || !shaderFactory) {
            updateFlag.clear();
            return;
        }

        bool is3D = mapInterface->is3d();

        // Just insert pointers here since we will only access the objects inside this method where we know that latestTileInfos is retained
        std::vector<const Tiled2dMapVectorTileInfo*> tilesToAdd;
        std::vector<const Tiled2dMapVectorTileInfo*> tilesToKeep;
        std::unordered_set<Tiled2dMapVersionedTileInfo> tilesToRemove;
        std::unordered_map<Tiled2dMapVersionedTileInfo, TileState> tileStateUpdates;
        std::unordered_map<Tiled2dMapVersionedTileInfo, Tiled2dMapLayerMaskWrapper> newTileMasks;

        {
            std::lock_guard<std::recursive_mutex> updateLock(updateMutex);
            updateFlag.clear();

            for (const auto &vectorTileInfo: latestTileInfos) {
                if (tiles.count(vectorTileInfo.tileInfo) == 0) {
                    tilesToAdd.push_back(&vectorTileInfo);
                } else {
                    tilesToKeep.push_back(&vectorTileInfo);
                    if (vectorTileInfo.state == TileState::IN_SETUP) {
                        // Tile has been cleared in source, but is still available here
                        if (tilesReadyControlSet.find(vectorTileInfo.tileInfo) == tilesReadyControlSet.end()) {
                            // Tile is ready - propagate to source
                            readyManager.message(MFN(&Tiled2dMapVectorReadyManager::didProcessData), readyManagerIndex,
                                                 vectorTileInfo.tileInfo, 0);
                        }
                    }
                }
            }

            for (const auto &[tileInfo, _]: tiles) {
                bool found = false;
                for (const auto &currentTile: latestTileInfos) {
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

                auto tileStateIt = tileStateMap.find(tileEntry->tileInfo);
                if (tileStateIt == tileStateMap.end() || tileStateIt->second != tileEntry->state) {
                    tileStateUpdates[tileEntry->tileInfo] = tileEntry->state;
                }

                size_t existingPolygonHash = 0;
                auto it = tileMaskMap.find(tileEntry->tileInfo);
                if (it != tileMaskMap.end()) {
                    existingPolygonHash = it->second.getPolygonHash();
                }

                const size_t hash = std::hash<std::vector<::PolygonCoord>>()(tileEntry->masks);

                if (hash != existingPolygonHash) {

                    const auto &tileMask = std::make_shared<PolygonMaskObject>(graphicsFactory,
                                                                               coordinateConverterHelper,
                                                                               is3D);
                    auto convertedTileBounds = coordinateConverterHelper->convertRectToRenderSystem(tileEntry->tileInfo.tileInfo.bounds);
                    
                    double cx = (convertedTileBounds.bottomRight.x + convertedTileBounds.topLeft.x) / 2.0;
                    double cy = (convertedTileBounds.bottomRight.y + convertedTileBounds.topLeft.y) / 2.0;
                    double rx = is3D ? 1.0 * sin(cy) * cos(cx) : cx;
                    double ry = is3D ? 1.0 * cos(cy): cy;
                    double rz = is3D ? -1.0 * sin(cy) * sin(cx) : 0.0;

                    Vec3D origin(rx, ry, rz);

                    tileMask->setPolygons(tileEntry->masks, origin, float(POLYGON_MASK_SUBDIVISION_FACTOR));

                    newTileMasks[tileEntry->tileInfo] = Tiled2dMapLayerMaskWrapper(tileMask, hash);
                }
            }

            if (tilesToAdd.empty() && tilesToRemove.empty() && newTileMasks.empty() && tileStateUpdates.empty()) return;

            for (const auto &tile: tilesToAdd) {

                std::unordered_set<int32_t> indexControlSet;

                tiles[tile->tileInfo] = {};
                assert(tileStateMap.count(tile->tileInfo) == 0);
                tileStateUpdates[tile->tileInfo] = tile->state;

                for (int32_t index = 0; index < mapDescription->layers.size(); index++) {
                    auto const &layer = mapDescription->layers.at(index);
                    if (layer->source != sourceName) {
                        continue;
                    }
                    auto const mapIt = tile->layerFeatureMaps->find(layer->sourceLayer);
                    if (mapIt == tile->layerFeatureMaps->end()) {
                        continue;
                    }

                    if ((tile->tileInfo.tileInfo.zoomIdentifier < layer->minZoom ||
                         tile->tileInfo.tileInfo.zoomIdentifier > layer->maxZoom) && tile->tileInfo.tileInfo.zoomIdentifier != layer->sourceMaxZoom) {
                        continue;
                    }

                    std::string identifier = layer->identifier;
                    Actor<Tiled2dMapVectorTile> actor = createTileActor(tile->tileInfo, layer);

                    if (actor) {
                        if (auto strongSelectionDelegate = selectionDelegate.lock()) {
                            actor.message(MFN(&Tiled2dMapVectorTile::setSelectionDelegate), strongSelectionDelegate);
                        }

                        indexControlSet.insert(index);
                        tiles[tile->tileInfo].push_back({index, identifier, actor.strongActor<Tiled2dMapVectorTile>()});

                        actor.message(MFN(&Tiled2dMapVectorTile::setVectorTileData), mapIt->second);
                    }
                }

                if (!indexControlSet.empty()) {
                    tilesReadyControlSet[tile->tileInfo] = indexControlSet;
                }

                readyManager.message(MFN(&Tiled2dMapVectorReadyManager::didProcessData), readyManagerIndex, tile->tileInfo,
                                     indexControlSet.empty() ? 0 : 1);
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


void Tiled2dMapVectorSourceVectorTileDataManager::onTileCompletelyReady(const Tiled2dMapVersionedTileInfo &tileInfo) {
    readyManager.message(MFN(&Tiled2dMapVectorReadyManager::setReady), readyManagerIndex, tileInfo, 1);
}

void Tiled2dMapVectorSourceVectorTileDataManager::updateLayerDescription(std::shared_ptr<VectorLayerDescription> layerDescription,
                                                                   int32_t legacyIndex,
                                                                   bool needsTileReplace) {
    auto mapInterface = this->mapInterface.lock();
    if (!mapInterface) {
        return;
    }

    for (const auto &tileData: latestTileInfos) {
        auto subTiles = tiles.find(tileData.tileInfo);
        if (subTiles == tiles.end()) {
            continue;
        }

        if (needsTileReplace) {
            std::vector<Actor<Tiled2dMapVectorTile>> tilesToClear;
            // Remove invalid legacy tile (only one - identifier is unique)
            auto legacyPos = std::find_if(subTiles->second.begin(), subTiles->second.end(),
                                          [&identifier = layerDescription->identifier]
                                                  (const std::tuple<int32_t, std::string, Actor<Tiled2dMapVectorTile>> &subTile) {
                                              return std::get<1>(subTile) == identifier;
                                          });
            if (legacyPos == subTiles->second.end()) {
                continue;
            }
            tilesToClear.push_back(std::get<2>(*legacyPos));
            subTiles->second.erase(legacyPos);

            // If new source of layer is not handled by this manager, continue
            if (layerDescription->source != source) {
                continue;
            }

            auto const mapIt = tileData.layerFeatureMaps->find(layerDescription->sourceLayer);
            if (mapIt == tileData.layerFeatureMaps->end()) {
                continue;
            }

            Actor<Tiled2dMapVectorTile> actor = createTileActor(tileData.tileInfo, layerDescription);
            if (actor) {
                if (auto strongSelectionDelegate = selectionDelegate.lock()) {
                    actor.message(MFN(&Tiled2dMapVectorTile::setSelectionDelegate), strongSelectionDelegate);
                }

                if (subTiles->second.empty()) {
                    subTiles->second.push_back({legacyIndex, layerDescription->identifier, actor.strongActor<Tiled2dMapVectorTile>()});
                } else {
                    auto insertionPoint = std::lower_bound(subTiles->second.begin(), subTiles->second.end(), legacyIndex, [](const auto& subTile, int index) {
                        return std::get<0>(subTile) < index;
                    });

                    subTiles->second.insert(insertionPoint, {legacyIndex, layerDescription->identifier, actor.strongActor<Tiled2dMapVectorTile>()});
                }

                auto controlSetEntry = tilesReadyControlSet.find(tileData.tileInfo);
                if (controlSetEntry == tilesReadyControlSet.end()) {
                    tilesReadyControlSet[tileData.tileInfo] = {legacyIndex};
                } else {
                    controlSetEntry->second.insert(legacyIndex);
                }
                tilesReady.erase(tileData.tileInfo);

                actor.message(MFN(&Tiled2dMapVectorTile::setVectorTileData), mapIt->second);
                auto castedMe = std::static_pointer_cast<Tiled2dMapVectorSourceVectorTileDataManager>(shared_from_this());
                auto selfActor = WeakActor<Tiled2dMapVectorSourceVectorTileDataManager>(mailbox, castedMe);
                selfActor.message(MailboxExecutionEnvironment::graphics, MFN(&Tiled2dMapVectorSourceVectorTileDataManager::clearTiles), tilesToClear);
            }

        } else {
            for (auto &[index, identifier, tile]  : subTiles->second) {
                if (identifier == layerDescription->identifier) {
                    auto const mapIt = tileData.layerFeatureMaps->find(layerDescription->sourceLayer);
                    if (mapIt == tileData.layerFeatureMaps->end()) {
                        break;
                    }

                    auto controlSetEntry = tilesReadyControlSet.find(tileData.tileInfo);
                    if (controlSetEntry == tilesReadyControlSet.end()) {
                        tilesReadyControlSet[tileData.tileInfo] = {legacyIndex};
                    } else {
                        controlSetEntry->second.insert(legacyIndex);
                    }
                    tilesReady.erase(tileData.tileInfo);

                    tile.message(MFN(&Tiled2dMapVectorTile::updateVectorLayerDescription), layerDescription, mapIt->second);
                    break;
                }
            }
        }
    }
}

void Tiled2dMapVectorSourceVectorTileDataManager::reloadLayerContent(const std::vector<std::tuple<std::shared_ptr<VectorLayerDescription>, int32_t>> &descriptionLayerIndexPairs) {
    auto mapInterface = this->mapInterface.lock();
    if (!mapInterface) {
        return;
    }

    for (const auto &[layerDescription, layerIndex]: descriptionLayerIndexPairs) {

        for (const auto &tileData: latestTileInfos) {
            auto subTiles = tiles.find(tileData.tileInfo);
            if (subTiles == tiles.end()) {
                continue;
            }

            std::vector<Actor<Tiled2dMapVectorTile>> tilesToClear;
            // Remove invalid legacy tile (only one - identifier is unique)
            auto legacyPos = std::find_if(subTiles->second.begin(), subTiles->second.end(),
                                          [&identifier = layerDescription->identifier]
                                                  (const std::tuple<int32_t, std::string, Actor<Tiled2dMapVectorTile>> &subTile) {
                                              return std::get<1>(subTile) == identifier;
                                          });
            if (legacyPos == subTiles->second.end()) {
                continue;
            }
            tilesToClear.push_back(std::get<2>(*legacyPos));
            subTiles->second.erase(legacyPos);

            auto const mapIt = tileData.layerFeatureMaps->find(layerDescription->sourceLayer);
            if (mapIt == tileData.layerFeatureMaps->end()) {
                continue;
            }

            Actor<Tiled2dMapVectorTile> actor = createTileActor(tileData.tileInfo, layerDescription);
            if (actor) {
                if (auto strongSelectionDelegate = selectionDelegate.lock()) {
                    actor.unsafe()->setSelectionDelegate(strongSelectionDelegate);
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
                                            {layerIndex, layerDescription->identifier, actor.strongActor<Tiled2dMapVectorTile>()});
                }

                auto controlSetEntry = tilesReadyControlSet.find(tileData.tileInfo);
                if (controlSetEntry == tilesReadyControlSet.end()) {
                    tilesReadyControlSet[tileData.tileInfo] = {layerIndex};
                } else {
                    controlSetEntry->second.insert(layerIndex);
                }
                tilesReady.erase(tileData.tileInfo);

                actor.message(MFN(&Tiled2dMapVectorTile::setVectorTileData), mapIt->second);
                auto castedMe = std::static_pointer_cast<Tiled2dMapVectorSourceVectorTileDataManager>(shared_from_this());
                auto selfActor = WeakActor<Tiled2dMapVectorSourceVectorTileDataManager>(mailbox, castedMe);
                selfActor.message(MailboxExecutionEnvironment::graphics,
                                  MFN(&Tiled2dMapVectorSourceVectorTileDataManager::clearTiles), tilesToClear);
            }
        }
    }
}

void Tiled2dMapVectorSourceVectorTileDataManager::clearTiles(const std::vector<Actor<Tiled2dMapVectorTile>> &tilesToClear) {
    for (const auto &tile: tilesToClear) {
        tile.syncAccess([&](auto tileActor){
            tileActor->clear();
        });
    }
    auto mapInterface = this->mapInterface.lock();
    if (mapInterface) {
        mapInterface->invalidate();
        vectorLayer.message(MFN(&Tiled2dMapVectorLayer::invalidateTilesState));
    }
}

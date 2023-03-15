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

Tiled2dMapVectorSourceRasterTileDataManager::Tiled2dMapVectorSourceRasterTileDataManager(
        const WeakActor<Tiled2dMapVectorLayer> &vectorLayer,
        const std::shared_ptr<VectorMapDescription> &mapDescription,
        const std::string &source,
        const WeakActor<Tiled2dMapRasterSource> &rasterSource)
        : Tiled2dMapVectorSourceTileDataManager(vectorLayer, mapDescription, source),
          rasterSource(rasterSource) {}

void Tiled2dMapVectorSourceRasterTileDataManager::onRasterTilesUpdated(const std::string &layerName,
                                                                       std::unordered_set<Tiled2dMapRasterTileInfo> currentTileInfos) {
    // TODO
    //return;
    auto mapInterface = this->mapInterface.lock();
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
            // TODO: Remove: std::lock_guard<std::recursive_mutex> lock(tilesMutex);

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
                // TODO: Remove: std::lock_guard<std::recursive_mutex> lock(tileMaskMapMutex);
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
                // TODO: Remove: std::lock_guard<std::recursive_mutex> tilesReadyCountLock(tilesReadyCountMutex);
                tilesReadyCount[tile.tileInfo] = 0; // TODO: UBCM - set to number of subtiles
            }

            {
                // TODO: Remove: std::lock_guard<std::recursive_mutex> lock(tilesMutex);
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
                            // TODO: Remove: std::lock_guard<std::recursive_mutex> tilesReadyCountLock(tilesReadyCountMutex);
                            tilesReadyCount[tile.tileInfo] += 1; // TODO: UBCM remove
                        }


                        {
                            // TODO: Remove: std::lock_guard<std::recursive_mutex> lock(tilesMutex);
                            tiles[tile.tileInfo].push_back({index, identifier, actor.strongActor<Tiled2dMapVectorTile>()});
                        }

                        actor.message(&Tiled2dMapVectorTile::setTileData, nullptr, data);
                    }
                }
            }

            bool isAlreadyReady = false;
            {
                // TODO: Remove: std::lock_guard<std::recursive_mutex> tilesReadyCountLock(tilesReadyCountMutex);
                isAlreadyReady = tilesReadyCount[tile.tileInfo] == 0;
            }
            if (isAlreadyReady) {
                rasterSource.message(&Tiled2dMapRasterSource::setTileReady, tile.tileInfo);
            }
        }

        if (!(newTileMasks.empty() && tilesToRemove.empty())) {
            auto castedMe = std::static_pointer_cast<Tiled2dMapVectorSourceTileDataManager>(shared_from_this());
            auto selfActor = WeakActor<Tiled2dMapVectorSourceTileDataManager>(mailbox, castedMe);
            selfActor.message(MailboxExecutionEnvironment::graphics, &Tiled2dMapVectorSourceTileDataManager::updateMaskObjects, newTileMasks, tilesToRemove);
        }
    }
    mapInterface->invalidate();

}

void Tiled2dMapVectorSourceRasterTileDataManager::onTileCompletelyReady(const Tiled2dMapTileInfo tileInfo) {
    rasterSource.message(&Tiled2dMapRasterSource::setTileReady, tileInfo);
}

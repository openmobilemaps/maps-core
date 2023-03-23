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

Tiled2dMapVectorSourceVectorTileDataManager::Tiled2dMapVectorSourceVectorTileDataManager(
        const WeakActor<Tiled2dMapVectorLayer> &vectorLayer,
        const std::shared_ptr<VectorMapDescription> &mapDescription,
        const std::string &source,
        const WeakActor<Tiled2dMapVectorSource> &vectorSource)
        : Tiled2dMapVectorSourceTileDataManager(vectorLayer, mapDescription, source),
          vectorSource(vectorSource) {

}

void Tiled2dMapVectorSourceVectorTileDataManager::onVectorTilesUpdated(const std::string &sourceName,
                                                                       std::unordered_set<Tiled2dMapVectorTileInfo> currentTileInfos) {
    auto mapInterface = this->mapInterface.lock();
    {
        auto graphicsFactory = mapInterface ? mapInterface->getGraphicsObjectFactory() : nullptr;
        auto coordinateConverterHelper = mapInterface ? mapInterface->getCoordinateConverterHelper() : nullptr;
        auto shaderFactory = mapInterface ? mapInterface->getShaderFactory() : nullptr;
        if (!graphicsFactory || !shaderFactory) {
            return;
        }

        // Just insert pointers here since we will only access the objects inside this method where we know that currentTileInfos is retained
        std::vector<const Tiled2dMapVectorTileInfo*> tilesToAdd;
        std::vector<const Tiled2dMapVectorTileInfo*> tilesToKeep;

        std::unordered_set<Tiled2dMapTileInfo> tilesToRemove;

        for (const auto &vectorTileInfo: currentTileInfos) {
            if (tiles.count(vectorTileInfo.tileInfo) == 0) {
                tilesToAdd.push_back(&vectorTileInfo);
            } else {
                tilesToKeep.push_back(&vectorTileInfo);
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

        std::unordered_map<Tiled2dMapTileInfo, Tiled2dMapLayerMaskWrapper> newTileMasks;
        for (const auto &tileEntry : tilesToKeep) {

            size_t existingPolygonHash = 0;
            auto it = tileMaskMap.find(tileEntry->tileInfo);
            if (it != tileMaskMap.end()) {
                existingPolygonHash = it->second.getPolygonHash();
            }

            const size_t hash = std::hash<std::vector<::PolygonCoord>>()(tileEntry->masks);

            if (hash != existingPolygonHash) {

                const auto &tileMask = std::make_shared<PolygonMaskObject>(graphicsFactory,
                                                                           coordinateConverterHelper);

                tileMask->setPolygons(tileEntry->masks);

                newTileMasks[tileEntry->tileInfo] = Tiled2dMapLayerMaskWrapper(tileMask, hash);
            }
        }

        if (tilesToAdd.empty() && tilesToRemove.empty() && newTileMasks.empty()) return;

        for (const auto &tile : tilesToAdd) {

            std::unordered_set<int32_t> indexControlSet;

            tiles[tile->tileInfo] = {};

            for (int32_t index = 0; index < mapDescription->layers.size(); index++) {
                auto const &layer= mapDescription->layers.at(index);
                if (layer->source != sourceName) {
                    continue;
                }
                if (!(layer->minZoom <= tile->tileInfo.zoomIdentifier && layer->maxZoom >= tile->tileInfo.zoomIdentifier)) {
                    continue;
                }
                auto const mapIt = tile->layerFeatureMaps->find(layer->sourceId);
                if (mapIt == tile->layerFeatureMaps->end()) {
                    continue;
                }

                std::string identifier = layer->identifier;
                Actor<Tiled2dMapVectorTile> actor = createTileActor(tile->tileInfo, layer);

                if (actor) {
                    if (selectionDelegate) {
                        actor.message(&Tiled2dMapVectorTile::setSelectionDelegate, selectionDelegate);
                    }

                    indexControlSet.insert(index);
                    tiles[tile->tileInfo].push_back({index, identifier, actor.strongActor<Tiled2dMapVectorTile>()});

                    actor.message(&Tiled2dMapVectorTile::setVectorTileData, mapIt->second);
                }
            }

            if (indexControlSet.empty()) {
                vectorSource.message(&Tiled2dMapVectorSource::setTileReady, tile->tileInfo);
            } else {
                tilesReadyControlSet[tile->tileInfo] = indexControlSet;
            }
        }

        if (!(newTileMasks.empty() && tilesToRemove.empty())) {
            auto castedMe = std::static_pointer_cast<Tiled2dMapVectorSourceTileDataManager>(shared_from_this());
            auto selfActor = WeakActor<Tiled2dMapVectorSourceTileDataManager>(mailbox, castedMe);
            selfActor.messagePrecisely(MailboxDuplicationStrategy::replaceNewest, MailboxExecutionEnvironment::graphics, &Tiled2dMapVectorSourceTileDataManager::updateMaskObjects, newTileMasks, tilesToRemove);
        }
    }
    mapInterface->invalidate();
}


void Tiled2dMapVectorSourceVectorTileDataManager::onTileCompletelyReady(const Tiled2dMapTileInfo tileInfo) {
    vectorSource.message(&Tiled2dMapVectorSource::setTileReady, tileInfo);
    auto mapInterface = this->mapInterface.lock();
    if (mapInterface) {
        mapInterface->invalidate();
    }
}

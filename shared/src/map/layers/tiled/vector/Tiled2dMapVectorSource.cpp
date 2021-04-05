/*
 * Copyright (c) 2021 Ubique Innovation AG <https://www.ubique.ch>
 *
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this
 *  file, You can obtain one at https://mozilla.org/MPL/2.0/.
 *
 *  SPDX-License-Identifier: MPL-2.0
 */


#include "Tiled2dMapVectorSource.h"

Tiled2dMapVectorSource::Tiled2dMapVectorSource(const MapConfig &mapConfig,
                                               const std::shared_ptr<Tiled2dMapLayerConfig> &layerConfig,
                                               const std::shared_ptr<CoordinateConversionHelperInterface> &conversionHelper,
                                               const std::shared_ptr<SchedulerInterface> &scheduler,
                                               const std::shared_ptr<::TileLoaderInterface> & tileLoader,
                                               const std::shared_ptr<Tiled2dMapSourceListenerInterface> &listener)
        : Tiled2dMapSource<VectorTileHolderInterface, VectorTileLoaderResult>(mapConfig, layerConfig, conversionHelper, scheduler,
                                                                              listener), loader(tileLoader) {}

VectorTileLoaderResult Tiled2dMapVectorSource::loadTile(Tiled2dMapTileInfo tile) {
    return loader->loadVectorTile(layerConfig->getTileUrl(tile.x, tile.y, tile.zoomIdentifier));
}

std::unordered_set<Tiled2dMapVectorTileInfo> Tiled2dMapVectorSource::getCurrentTiles() {
    std::lock_guard<std::recursive_mutex> lock(tilesMutex);
    std::unordered_set<Tiled2dMapVectorTileInfo> currentTileInfos;
    for (const auto &tileEntry : currentTiles) {
        currentTileInfos.insert(Tiled2dMapVectorTileInfo(tileEntry.first, tileEntry.second));
    }
    return currentTileInfos;
}

void Tiled2dMapVectorSource::pause() {
    // TODO: Stop loading tiles
}

void Tiled2dMapVectorSource::resume() {
    // TODO: Reload textures of current tiles
}
/*
 * Copyright (c) 2021 Ubique Innovation AG <https://www.ubique.ch>
 *
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this
 *  file, You can obtain one at https://mozilla.org/MPL/2.0/.
 *
 *  SPDX-License-Identifier: MPL-2.0
 */

#include "Tiled2dMapRasterSource.h"
#include "LambdaTask.h"
#include <algorithm>
#include <cmath>
#include <string>

Tiled2dMapRasterSource::Tiled2dMapRasterSource(const MapConfig &mapConfig,
                                               const std::shared_ptr<Tiled2dMapLayerConfig> &layerConfig,
                                               const std::shared_ptr<CoordinateConversionHelperInterface> &conversionHelper,
                                               const std::shared_ptr<SchedulerInterface> &scheduler,
                                               const std::shared_ptr<::LoaderInterface> &textureLoader,
                                               const std::shared_ptr<Tiled2dMapSourceListenerInterface> &listener,
                                               float screenDensityPpi)
    : Tiled2dMapSource<TextureHolderInterface, TextureLoaderResult, std::shared_ptr<::TextureHolderInterface>>(
          mapConfig, layerConfig, conversionHelper, scheduler, listener, screenDensityPpi)
    , loader(textureLoader) {}

TextureLoaderResult Tiled2dMapRasterSource::loadTile(Tiled2dMapTileInfo tile) {
    return loader->loadTexture(layerConfig->getTileUrl(tile.x, tile.y, tile.zoomIdentifier), std::nullopt);
}

std::shared_ptr<::TextureHolderInterface> Tiled2dMapRasterSource::postLoadingTask(const TextureLoaderResult &loadedData,
                                                                                  const Tiled2dMapTileInfo &tile) {
    return loadedData.data;
}

std::unordered_set<Tiled2dMapRasterTileInfo> Tiled2dMapRasterSource::getCurrentTiles() {
    std::lock_guard<std::recursive_mutex> lock(tilesMutex);
    std::unordered_set<Tiled2dMapRasterTileInfo> currentTileInfos;
    for (const auto &tileEntry : currentTiles) {
        currentTileInfos.insert(Tiled2dMapRasterTileInfo(tileEntry.first, tileEntry.second));
    }
    return currentTileInfos;
}

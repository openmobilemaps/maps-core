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
                                               const std::vector<std::shared_ptr<::LoaderInterface>> & loaders,
                                               const WeakActor<Tiled2dMapRasterSourceListener> &listener,
                                               float screenDensityPpi)
    : Tiled2dMapSource<TextureHolderInterface, TextureLoaderResult, std::shared_ptr<::TextureHolderInterface>>(
          mapConfig, layerConfig, conversionHelper, scheduler, screenDensityPpi, loaders.size())
, loaders(loaders)
, rasterLayerActor(listener){}

void Tiled2dMapRasterSource::cancelLoad(Tiled2dMapTileInfo tile, size_t loaderIndex) {
    std::string const url = layerConfig->getTileUrl(tile.x, tile.y, tile.t, tile.zoomIdentifier);
    loaders[loaderIndex]->cancel(url);
}

::djinni::Future<TextureLoaderResult> Tiled2dMapRasterSource::loadDataAsync(Tiled2dMapTileInfo tile, size_t loaderIndex) {
    std::string const url = layerConfig->getTileUrl(tile.x, tile.y, tile.t, tile.zoomIdentifier);
    return loaders[loaderIndex]->loadTextureAsnyc(url, std::nullopt);
}

bool Tiled2dMapRasterSource::hasExpensivePostLoadingTask() {
    return false;
}

std::shared_ptr<::TextureHolderInterface> Tiled2dMapRasterSource::postLoadingTask(const TextureLoaderResult &loadedData,
                                                                                  const Tiled2dMapTileInfo &tile) {
    return loadedData.data;
}

void Tiled2dMapRasterSource::notifyTilesUpdates() {
    rasterLayerActor.message(MailboxDuplicationStrategy::replaceNewest, &Tiled2dMapRasterSourceListener::onTilesUpdated,
                             layerConfig->getLayerName(), getCurrentTiles());
}

std::unordered_set<Tiled2dMapRasterTileInfo> Tiled2dMapRasterSource::getCurrentTiles() {
    std::unordered_set<Tiled2dMapRasterTileInfo> currentTileInfos;
    currentTileInfos.reserve(currentTiles.size());
    std::transform(currentTiles.rbegin(), currentTiles.rend(), std::inserter(currentTileInfos, currentTileInfos.end()),
        [](const auto& tilePair) {
            const auto& [tileInfo, tileWrapper] = tilePair;
            return Tiled2dMapRasterTileInfo(std::move(tileInfo), std::move(tileWrapper.result), std::move(tileWrapper.masks), std::move(tileWrapper.state));
        }
    );
    return currentTileInfos;
}

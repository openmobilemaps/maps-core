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
#include "Tiled2dMapRasterLayer.h"
#include "LambdaTask.h"
#include <algorithm>
#include <cmath>
#include <string>

Tiled2dMapRasterSource::Tiled2dMapRasterSource(const MapConfig &mapConfig,
                                               const std::shared_ptr<Tiled2dMapLayerConfig> &layerConfig,
                                               const std::shared_ptr<CoordinateConversionHelperInterface> &conversionHelper,
                                               const std::shared_ptr<SchedulerInterface> &scheduler,
                                               const std::vector<std::shared_ptr<::LoaderInterface>> & loaders,
                                               const WeakActor<Tiled2dMapRasterLayer> &listener,
                                               float screenDensityPpi)
    : Tiled2dMapSource<TextureHolderInterface, TextureLoaderResult, std::shared_ptr<::TextureHolderInterface>>(
          mapConfig, layerConfig, conversionHelper, scheduler, listener.weakActor<Tiled2dMapSourceListenerInterface>(), screenDensityPpi, loaders.size())
, loaders(loaders)
, rasterLayerActor(listener){}

TextureLoaderResult Tiled2dMapRasterSource::loadTile(Tiled2dMapTileInfo tile, size_t loaderIndex) {
    std::weak_ptr<Tiled2dMapSource> weakSelfPtr = std::dynamic_pointer_cast<Tiled2dMapRasterSource>(shared_from_this());
    
    auto weakActor = WeakActor<Tiled2dMapRasterSource>(mailbox, std::static_pointer_cast<Tiled2dMapRasterSource>(shared_from_this()));
    scheduler->addTask(std::make_shared<LambdaTask>(TaskConfig("", 0, TaskPriority::NORMAL, ExecutionEnvironment::IO), [&, weakActor, tile, loaderIndex] {
        auto result = loaders[loaderIndex]->loadTexture(layerConfig->getTileUrl(tile.x, tile.y, tile.t, tile.zoomIdentifier), std::nullopt);
        weakActor.message(&Tiled2dMapRasterSource::didLoad, tile, loaderIndex, result);
    }));
    return TextureLoaderResult(nullptr, std::nullopt, LoaderStatus::OK, std::nullopt);
}

void Tiled2dMapRasterSource::cancelLoad(Tiled2dMapTileInfo tile, size_t loaderIndex) {
    std::string const url = layerConfig->getTileUrl(tile.x, tile.y, tile.t, tile.zoomIdentifier);
    loaders[loaderIndex]->cancel(url);
}

::djinni::Future<TextureLoaderResult> Tiled2dMapRasterSource::loadDataAsync(Tiled2dMapTileInfo tile, size_t loaderIndex) {
    std::string const url = layerConfig->getTileUrl(tile.x, tile.y, tile.t, tile.zoomIdentifier);
    return loaders[loaderIndex]->loadTextureAsnyc(url, std::nullopt);
}

std::shared_ptr<::TextureHolderInterface> Tiled2dMapRasterSource::postLoadingTask(const TextureLoaderResult &loadedData,
                                                                                  const Tiled2dMapTileInfo &tile) {
    return loadedData.data;
}

void Tiled2dMapRasterSource::notifyTilesUpdates() {
    rasterLayerActor.message(&Tiled2dMapRasterLayer::onTilesUpdatedNew, getCurrentTiles());
}

std::unordered_set<Tiled2dMapRasterTileInfo> Tiled2dMapRasterSource::getCurrentTiles() {
    std::unordered_set<Tiled2dMapRasterTileInfo> currentTileInfos;
    for (auto it = currentTiles.rbegin(); it != currentTiles.rend(); it++ ) {
        auto &[tileInfo, tileWrapper] = *it;
        if (tileWrapper.isVisible) {
            currentTileInfos.insert(Tiled2dMapRasterTileInfo(tileInfo, tileWrapper.result, tileWrapper.masks));
        }
    }
    return currentTileInfos;
}

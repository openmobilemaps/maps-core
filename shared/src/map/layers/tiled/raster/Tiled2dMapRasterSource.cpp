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
                                               float screenDensityPpi, 
                                               const std::shared_ptr<Tiled2dMapLayerConfig> &heightLayerConfig)
    : Tiled2dMapSource<TextureHolderInterface, std::pair<TextureLoaderResult, std::optional<TextureLoaderResult>>, std::pair<std::shared_ptr<::TextureHolderInterface>, std::shared_ptr<::TextureHolderInterface>>>(
          mapConfig, layerConfig, heightLayerConfig, conversionHelper, scheduler, screenDensityPpi, loaders.size())
, loaders(loaders)
, rasterLayerActor(listener), heightLayerConfig(heightLayerConfig) {}

void Tiled2dMapRasterSource::cancelLoad(Tiled2dMapTileInfo tile, size_t loaderIndex) {
    std::string const url = layerConfig->getTileUrl(tile.x, tile.y, tile.t, tile.zoomIdentifier);
    loaders[loaderIndex]->cancel(url);
    if (heightLayerConfig) {
        std::string const heightUrl = heightLayerConfig->getTileUrl(tile.x, tile.y, tile.t, tile.zoomIdentifier);
        loaders[loaderIndex]->cancel(heightUrl);
    }
}

LoaderStatus Tiled2dMapRasterSource::getLoaderStatus(const std::pair<TextureLoaderResult, std::optional<TextureLoaderResult>> &loaderResult) {
    if (loaderResult.second && loaderResult.first.status == LoaderStatus::OK) {
//        return LoaderStatus::OK;
        return loaderResult.second->status;
    }
    return loaderResult.first.status;
}

std::optional<std::string> Tiled2dMapRasterSource::getErrorCode(const std::pair<TextureLoaderResult, std::optional<TextureLoaderResult>> &loaderResult) {
    if (loaderResult.second && loaderResult.first.status == LoaderStatus::OK) {
        return loaderResult.second->errorCode;
    }
    return loaderResult.first.errorCode;
}

::djinni::Future<std::pair<TextureLoaderResult, std::optional<TextureLoaderResult>>> Tiled2dMapRasterSource::loadDataAsync(Tiled2dMapTileInfo tile, size_t loaderIndex) {
    std::string const url = layerConfig->getTileUrl(tile.x, tile.y, tile.t, tile.zoomIdentifier);
    auto futureResult = loaders[loaderIndex]->loadTextureAsnyc(url, std::nullopt);

    if (heightLayerConfig) {


        struct Context {
            std::atomic<size_t> counter;
            djinni::Promise<std::pair<TextureLoaderResult, std::optional<TextureLoaderResult>>> promise;
            std::optional<TextureLoaderResult> loaderResult;
            std::optional<TextureLoaderResult> heightLoaderResult;
            Context() : counter(2) {}
        };

        auto context = std::make_shared<Context>();
        auto future = context->promise.getFuture();

        futureResult.then([context](auto r) {
            context->loaderResult = r.get();
            if (--(context->counter) == 0) {
                std::pair<TextureLoaderResult, std::optional<TextureLoaderResult>> combined = std::make_pair(context->loaderResult.value(), context->heightLoaderResult);
                context->promise.setValue(combined);
            }
        });

        std::string const heightUrl = heightLayerConfig->getTileUrl(tile.x, tile.y, tile.t, tile.zoomIdentifier);
        auto futureHeightResult = loaders[loaderIndex]->loadTextureAsnyc(heightUrl, std::nullopt);

        futureHeightResult.then([context](auto r) {
            context->heightLoaderResult = r.get();
            if (--(context->counter) == 0) {
                std::pair<TextureLoaderResult, std::optional<TextureLoaderResult>> combined = std::make_pair(context->loaderResult.value(), context->heightLoaderResult);
                context->promise.setValue(combined);
            }
        });





        return future;

    }


    return futureResult.then([](::djinni::Future<TextureLoaderResult> result) -> std::pair<TextureLoaderResult, std::optional<TextureLoaderResult>> {
        return std::make_pair(result.get(), std::nullopt);
    });


}

std::pair<std::shared_ptr<::TextureHolderInterface>, std::shared_ptr<::TextureHolderInterface>> Tiled2dMapRasterSource::postLoadingTask(const std::pair<TextureLoaderResult, std::optional<TextureLoaderResult>> &loadedData,
                                                                                  const Tiled2dMapTileInfo &tile) {
    if (loadedData.second) {
        return std::make_pair(loadedData.first.data, loadedData.second.value().data);
    } else {
        return std::make_pair(loadedData.first.data, nullptr);
    }

}

void Tiled2dMapRasterSource::notifyTilesUpdates() {
    rasterLayerActor.message(MailboxDuplicationStrategy::replaceNewest, &Tiled2dMapRasterSourceListener::onTilesUpdated,
                             layerConfig->getLayerName(), getCurrentTiles());
}

std::unordered_set<Tiled2dMapRasterTileInfo> Tiled2dMapRasterSource::getCurrentTiles() {
    std::unordered_set<Tiled2dMapRasterTileInfo> currentTileInfos;
    for (auto it = currentTiles.rbegin(); it != currentTiles.rend(); it++ ) {
        auto &[tileInfo, tileWrapper] = *it;
        if (tileWrapper.isVisible) {
            currentTileInfos.insert(Tiled2dMapRasterTileInfo(tileInfo, tileWrapper.result.first, tileWrapper.result.second, tileWrapper.masks, tileWrapper.targetZoomLevelOffset));
        }
    }
    return currentTileInfos;
}

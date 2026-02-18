/*
 * Copyright (c) 2021 Ubique Innovation AG <https://www.ubique.ch>
 *
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this
 *  file, You can obtain one at https://mozilla.org/MPL/2.0/.
 *
 *  SPDX-License-Identifier: MPL-2.0
 */

#include "TiledDisplacedRasterSource.h"
#include <functional>
#include <mutex>
#include <unordered_set>

namespace {
    struct DualTileLoadState {
        explicit DualTileLoadState(const std::shared_ptr<::djinni::Promise<std::shared_ptr<TextureLoaderResult>>> &promise,
                                   std::function<void(const TextureLoaderResult &, const TextureLoaderResult &)> onBothOk)
            : promise(promise)
            , onBothOk(std::move(onBothOk)) {}

        void complete(const TextureLoaderResult &result, bool isElevationResult) {
            std::lock_guard<std::mutex> lock(mutex);
            if (resolved) {
                return;
            }

            if (isElevationResult) {
                elevationResult = result;
                if (result.status != LoaderStatus::OK) {
                    resolved = true;
                    promise->setValue(std::make_shared<TextureLoaderResult>(result));
                    return;
                }
            } else {
                rasterResult = result;
                if (result.status != LoaderStatus::OK) {
                    resolved = true;
                    promise->setValue(std::make_shared<TextureLoaderResult>(result));
                    return;
                }
            }

            if (rasterResult.has_value() && elevationResult.has_value()) {
                onBothOk(*rasterResult, *elevationResult);
                resolved = true;
                promise->setValue(std::make_shared<TextureLoaderResult>(*rasterResult));
            }
        }

        std::shared_ptr<::djinni::Promise<std::shared_ptr<TextureLoaderResult>>> promise;
        std::mutex mutex;
        std::optional<TextureLoaderResult> rasterResult;
        std::optional<TextureLoaderResult> elevationResult;
        std::function<void(const TextureLoaderResult &, const TextureLoaderResult &)> onBothOk;
        bool resolved = false;
    };
}

TiledDisplacedRasterSource::TiledDisplacedRasterSource(const MapConfig &mapConfig,
                                                       const std::shared_ptr<Tiled2dMapLayerConfig> &layerConfig,
                                                       const std::shared_ptr<Tiled2dMapLayerConfig> &elevationConfig,
                                                       const std::shared_ptr<CoordinateConversionHelperInterface> &conversionHelper,
                                                       const std::shared_ptr<SchedulerInterface> &scheduler,
                                                       const std::vector<std::shared_ptr<::LoaderInterface>> &loaders,
                                                       const WeakActor<Tiled2dMapRasterSourceListener> &listener,
                                                       float screenDensityPpi,
                                                       std::string layerName)
    : Tiled2dMapRasterSource(mapConfig, layerConfig, conversionHelper, scheduler, loaders, listener, screenDensityPpi, layerName)
    , elevationConfig(elevationConfig) {}

void TiledDisplacedRasterSource::cancelLoad(Tiled2dMapTileInfo tile, size_t loaderIndex) {
    const std::string rasterUrl = layerConfig->getTileUrl(tile.x, tile.y, tile.t, tile.zoomIdentifier);
    const std::string elevationUrl = elevationConfig->getTileUrl(tile.x, tile.y, tile.t, tile.zoomIdentifier);

    loaders[loaderIndex]->cancel(rasterUrl);
    if (elevationUrl != rasterUrl) {
        loaders[loaderIndex]->cancel(elevationUrl);
    }
}

::djinni::Future<std::shared_ptr<TextureLoaderResult>> TiledDisplacedRasterSource::loadDataAsync(Tiled2dMapTileInfo tile,
                                                                                                   size_t loaderIndex) {
    const std::string rasterUrl = layerConfig->getTileUrl(tile.x, tile.y, tile.t, tile.zoomIdentifier);
    const std::string elevationUrl = elevationConfig->getTileUrl(tile.x, tile.y, tile.t, tile.zoomIdentifier);

    auto promise = std::make_shared<::djinni::Promise<std::shared_ptr<TextureLoaderResult>>>();
    auto state = std::make_shared<DualTileLoadState>(
        promise,
        [this, tile](const TextureLoaderResult &rasterResult, const TextureLoaderResult &elevationResult) {
            std::lock_guard<std::mutex> lock(elevationTextureHoldersMutex);
            if (rasterResult.status == LoaderStatus::OK && elevationResult.status == LoaderStatus::OK) {
                elevationTextureHolders[tile] = elevationResult.data;
            } else {
                elevationTextureHolders.erase(tile);
            }
        });

    loaders[loaderIndex]->loadTextureAsync(rasterUrl, std::nullopt).then(
        [state](::djinni::Future<::TextureLoaderResult> result) { state->complete(result.get(), false); });
    loaders[loaderIndex]->loadTextureAsync(elevationUrl, std::nullopt).then(
        [state](::djinni::Future<::TextureLoaderResult> result) { state->complete(result.get(), true); });

    return promise->getFuture();
}

void TiledDisplacedRasterSource::notifyTilesUpdates() {
    std::unordered_set<Tiled2dMapTileInfo> currentTileKeys;
    VectorSet<Tiled2dMapRasterTileInfo> currentTileInfos;
    currentTileInfos.reserve(currentTiles.size());

    std::lock_guard<std::mutex> lock(elevationTextureHoldersMutex);
    for (auto it = currentTiles.rbegin(); it != currentTiles.rend(); it++) {
        const auto &[tileInfo, tileWrapper] = *it;
        currentTileKeys.insert(tileInfo);

        auto elevationHolderIt = elevationTextureHolders.find(tileInfo);
        auto elevationHolder = elevationHolderIt != elevationTextureHolders.end() ? elevationHolderIt->second : nullptr;

        currentTileInfos.insert(Tiled2dMapRasterTileInfo(Tiled2dMapVersionedTileInfo(tileInfo, (size_t)tileWrapper.result.get()),
                                                         tileWrapper.result, tileWrapper.masks, tileWrapper.state,
                                                         tileWrapper.tessellationFactor, elevationHolder));
    }

    for (auto it = elevationTextureHolders.begin(); it != elevationTextureHolders.end();) {
        if (currentTileKeys.count(it->first) == 0) {
            it = elevationTextureHolders.erase(it);
        } else {
            ++it;
        }
    }

    rasterLayerActor.message(MailboxDuplicationStrategy::replaceNewest, MFN(&Tiled2dMapRasterSourceListener::onTilesUpdated),
                             layerConfig->getLayerName(), std::move(currentTileInfos));
}

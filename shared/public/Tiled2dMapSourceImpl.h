/*
 * Copyright (c) 2021 Ubique Innovation AG <https://www.ubique.ch>
 *
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this
 *  file, You can obtain one at https://mozilla.org/MPL/2.0/.
 *
 *  SPDX-License-Identifier: MPL-2.0
 */

#include "DateHelper.h"
#include "Tiled2dMapSource.h"
#include "TiledLayerError.h"
#include <algorithm>

#include "PolygonCoord.h"
#include <iostream>
#include "gpc.h"
#include "Logger.h"

template<class T, class L, class R>
Tiled2dMapSource<T, L, R>::Tiled2dMapSource(const MapConfig &mapConfig, const std::shared_ptr<Tiled2dMapLayerConfig> &layerConfig,
                                         const std::shared_ptr<CoordinateConversionHelperInterface> &conversionHelper,
                                         const std::shared_ptr<SchedulerInterface> &scheduler,
                                         float screenDensityPpi,
                                         size_t loaderCount,
                                         std::string layerName)
    : mapConfig(mapConfig)
    , layerConfig(layerConfig)
    , conversionHelper(conversionHelper)
    , scheduler(scheduler)
    , zoomLevelInfos(layerConfig->getZoomLevelInfos())
    , zoomInfo(layerConfig->getZoomInfo())
    , layerSystemId(layerConfig->getCoordinateSystemIdentifier())
    , isPaused(false)
    , screenDensityPpi(screenDensityPpi)
    , layerName(layerName) {
    std::sort(zoomLevelInfos.begin(), zoomLevelInfos.end(),
              [](const Tiled2dMapZoomLevelInfo &a, const Tiled2dMapZoomLevelInfo &b) -> bool { return a.zoom > b.zoom; });
}

const static double VIEWBOUNDS_PADDING_MIN_DIM_PC = 0.15;
const static int8_t ALWAYS_KEEP_LEVEL_TARGET_ZOOM_OFFSET = -8;

template<class T, class L, class R>
bool Tiled2dMapSource<T, L, R>::isTileVisible(const Tiled2dMapTileInfo &tileInfo) {
    return currentVisibleTiles.count(tileInfo) > 0;
}

template <typename T>
static void hash_combine(size_t& seed, const T& value) {
    std::hash<T> hasher;
    auto v = hasher(value);
    seed ^= v + 0x9e3779b9 + (seed << 6) + (seed >> 2);
}

template<class T, class L, class R>
void Tiled2dMapSource<T, L, R>::onVisibleBoundsChanged(const ::RectCoord &visibleBounds, int curT, double zoom) {
    if (isPaused) {
        return;
    }

    std::vector<PrioritizedTiled2dMapTileInfo> visibleTilesVec;

    RectCoord visibleBoundsLayer = conversionHelper->convertRect(layerSystemId, visibleBounds);

    const auto bounds = layerConfig->getBounds();

    double centerVisibleX = visibleBoundsLayer.topLeft.x + 0.5 * (visibleBoundsLayer.bottomRight.x - visibleBoundsLayer.topLeft.x);
    double centerVisibleY = visibleBoundsLayer.topLeft.y + 0.5 * (visibleBoundsLayer.bottomRight.y - visibleBoundsLayer.topLeft.y);

    size_t numZoomLevels = zoomLevelInfos.size();
    int targetZoomLayer = -1;

    // Each pixel is assumed to be 0.28mm – https://gis.stackexchange.com/a/315989
    const float screenScaleFactor = zoomInfo.adaptScaleToScreen ? screenDensityPpi / (0.0254 / 0.00028) : 1.0;

    if (!zoomInfo.underzoom
        && (zoomLevelInfos.empty() || zoomLevelInfos[0].zoom * zoomInfo.zoomLevelScaleFactor * screenScaleFactor < zoom)
        && (zoomLevelInfos.empty() || zoomLevelInfos[0].zoomLevelIdentifier != 0)) { // enable underzoom if the first zoomLevel is zoomLevelIdentifier == 0
        onVisibleTilesChanged({});
        return;
    }

    for (int i = 0; i < numZoomLevels; i++) {
        const Tiled2dMapZoomLevelInfo &zoomLevelInfo = zoomLevelInfos.at(i);
        if (zoomInfo.zoomLevelScaleFactor * screenScaleFactor * zoomLevelInfo.zoom < zoom) {
            targetZoomLayer = std::max(i - 1, 0);
            break;
        }
    }
    if (targetZoomLayer < 0) {
        if (!zoomInfo.overzoom) {
            onVisibleTilesChanged({});
            return;
        }
        targetZoomLayer = (int) numZoomLevels - 1;
    }
    int targetZoomLevelIdentifier = zoomLevelInfos.at(targetZoomLayer).zoomLevelIdentifier;
    int startZoomLayer = 0;
    int endZoomLevel = std::min((int) numZoomLevels - 1, targetZoomLayer + 2);
    int keepZoomLevelOffset = std::max(zoomLevelInfos.at(startZoomLayer).zoomLevelIdentifier,
                                       zoomLevelInfos.at(endZoomLevel).zoomLevelIdentifier + ALWAYS_KEEP_LEVEL_TARGET_ZOOM_OFFSET) -
                              targetZoomLevelIdentifier;

    int distanceWeight = 100;
    int zoomLevelWeight = 1000 * zoomLevelInfos.at(0).numTilesT;
    int zDistanceWeight = 1000 * zoomLevelInfos.at(0).numTilesT;

    std::vector<VisibleTilesLayer> layers;

    double visibleWidth = visibleBoundsLayer.bottomRight.x - visibleBoundsLayer.topLeft.x;
    double visibleHeight = visibleBoundsLayer.topLeft.y - visibleBoundsLayer.bottomRight.y;
    double viewboundsPadding = VIEWBOUNDS_PADDING_MIN_DIM_PC * std::min(std::abs(visibleWidth), std::abs(visibleHeight));

    double signWidth = visibleWidth / std::abs(visibleWidth);
    const double visibleLeft = visibleBoundsLayer.topLeft.x - signWidth * viewboundsPadding;
    const double visibleRight = visibleBoundsLayer.bottomRight.x + signWidth * viewboundsPadding;
    visibleWidth = std::abs(visibleWidth) + 2 * viewboundsPadding;

    double signHeight = visibleHeight / std::abs(visibleHeight);
    const double visibleTop = visibleBoundsLayer.topLeft.y + signHeight * viewboundsPadding;
    const double visibleBottom = visibleBoundsLayer.bottomRight.y - signHeight * viewboundsPadding;
    visibleHeight = std::abs(visibleHeight) + 2 * viewboundsPadding;



    size_t visibleTileHash = 0;

    for (int i = startZoomLayer; i <= endZoomLevel; i++) {
        const Tiled2dMapZoomLevelInfo &zoomLevelInfo = zoomLevelInfos.at(i);

        // If there is only one zoom level (zoomLevel 0), we disregard the min and max zoomLevel settings.
        // This is because a GeoJSON with only points inherently has zoomLevel 0, and restricting the zoom level
        // in such cases wouldn't be meaningful. Therefore, we skip the zoom level checks when startZoomLayer
        // and endZoomLevel are both zero.
        if (!(startZoomLayer == 0 && endZoomLevel == 0)) {
            if (minZoomLevelIdentifier.has_value() && zoomLevelInfo.zoomLevelIdentifier < minZoomLevelIdentifier) {
                continue;
            }
            if (maxZoomLevelIdentifier.has_value() && zoomLevelInfo.zoomLevelIdentifier > maxZoomLevelIdentifier) {
                continue;
            }
        }

        VisibleTilesLayer curVisibleTiles(i - targetZoomLayer);
        std::vector<PrioritizedTiled2dMapTileInfo> curVisibleTilesVec;

        const double tileWidth = zoomLevelInfo.tileWidthLayerSystemUnits;
        int zoomDistanceFactor = std::abs(zoomLevelInfo.zoomLevelIdentifier - targetZoomLevelIdentifier);

        RectCoord layerBounds = zoomLevelInfo.bounds;
        layerBounds = conversionHelper->convertRect(layerSystemId, layerBounds);

        const bool leftToRight = layerBounds.topLeft.x < layerBounds.bottomRight.x;
        const bool topToBottom = layerBounds.topLeft.y < layerBounds.bottomRight.y;
        const double tileWidthAdj = leftToRight ? tileWidth : -tileWidth;
        const double tileHeightAdj = topToBottom ? tileWidth : -tileWidth;

        const double boundsLeft = layerBounds.topLeft.x;
        int startTileLeft =
                std::floor(std::max(leftToRight ? (visibleLeft - boundsLeft) : (boundsLeft - visibleLeft), 0.0) / tileWidth);
        int maxTileLeft = std::floor(
                std::max(leftToRight ? (visibleRight - boundsLeft) : (boundsLeft - visibleRight), 0.0) / tileWidth);

        const double boundsTop = layerBounds.topLeft.y;
        int startTileTop = std::floor(std::max(topToBottom ? (visibleTop - boundsTop) : (boundsTop - visibleTop), 0.0) / tileWidth);
        int maxTileTop = std::floor(
                std::max(topToBottom ? (visibleBottom - boundsTop) : (boundsTop - visibleBottom), 0.0) / tileWidth);

        if (bounds.has_value()) {
            const auto converted = conversionHelper->convertRect(CoordinateSystemIdentifiers::EPSG3857(), *bounds);

            const double tLength = zoomLevelInfo.tileWidthLayerSystemUnits / 256;

            int min_left_pixel = floor((converted.topLeft.x - zoomLevelInfo.bounds.topLeft.x) / tLength);
            int min_left = std::max(0, min_left_pixel / 256);
            int max_top_pixel = floor((zoomLevelInfo.bounds.topLeft.y - converted.topLeft.y) / tLength);
            int max_top = std::min(zoomLevelInfo.numTilesY, max_top_pixel / 256);
            int max_left_pixel = floor((converted.bottomRight.x - zoomLevelInfo.bounds.topLeft.x) / tLength);
            int max_left = std::min(zoomLevelInfo.numTilesX, max_left_pixel / 256);
            int min_top_pixel = floor((zoomLevelInfo.bounds.topLeft.y - converted.bottomRight.y) / tLength);
            int min_top = min_top_pixel / 256;
            min_top = std::max(0, min_top);

            startTileLeft = std::max(min_left, startTileLeft);
            maxTileLeft = std::min(max_left, maxTileLeft);
            startTileTop = std::max(min_top, startTileTop);
            maxTileTop = std::min(max_top, maxTileTop);
        }

        const double maxDisCenterX = visibleWidth * 0.5 + tileWidth;
        const double maxDisCenterY = visibleHeight * 0.5 + tileWidth;
        const double maxDisCenter = std::sqrt(maxDisCenterX * maxDisCenterX + maxDisCenterY * maxDisCenterY);

        hash_combine(visibleTileHash, std::hash<int>{}(i));
        hash_combine(visibleTileHash, std::hash<int>{}(startTileLeft));
        hash_combine(visibleTileHash, std::hash<int>{}(maxTileLeft));
        hash_combine(visibleTileHash, std::hash<int>{}(startTileTop));
        hash_combine(visibleTileHash, std::hash<int>{}(maxTileTop));
        hash_combine(visibleTileHash, std::hash<int>{}(zoomLevelInfo.numTilesT));

        for (int x = startTileLeft; x <= maxTileLeft && x < zoomLevelInfo.numTilesX; x++) {
            for (int y = startTileTop; y <= maxTileTop && y < zoomLevelInfo.numTilesY; y++) {
                for (int t = 0; t < zoomLevelInfo.numTilesT; t++) {

                    if( t != curT ) {
                        continue;
                    }

                    const Coord topLeft = Coord(layerSystemId, x * tileWidthAdj + boundsLeft, y * tileHeightAdj + boundsTop, 0);
                    const Coord bottomRight = Coord(layerSystemId, topLeft.x + tileWidthAdj, topLeft.y + tileHeightAdj, 0);

                    const double tileCenterX = topLeft.x + 0.5f * tileWidthAdj;
                    const double tileCenterY = topLeft.y + 0.5f * tileHeightAdj;
                    const double tileCenterDis = std::sqrt(std::pow(tileCenterX - centerVisibleX, 2.0) + std::pow(tileCenterY - centerVisibleY, 2.0));

                    float distanceFactor = (tileCenterDis / maxDisCenter) * distanceWeight;
                    float zoomlevelFactor = zoomDistanceFactor * zoomLevelWeight;
                    float zDistanceFactor = std::abs(t - curT) * zDistanceWeight;

                    const int priority = std::ceil(distanceFactor + zoomlevelFactor + zDistanceFactor);

                    const RectCoord rect(topLeft, bottomRight);
                    curVisibleTilesVec.push_back(PrioritizedTiled2dMapTileInfo(
                            Tiled2dMapTileInfo(rect, x, y, t, zoomLevelInfo.zoomLevelIdentifier, zoomLevelInfo.zoom),
                            priority));

                    visibleTilesVec.push_back(curVisibleTilesVec.back());
                }
            }
        }

        curVisibleTiles.visibleTiles.insert(curVisibleTilesVec.begin(), curVisibleTilesVec.end());

        std::unordered_set<PrioritizedTiled2dMapTileInfo> visibleTiles(visibleTilesVec.begin(), visibleTilesVec.end());
        layers.push_back(curVisibleTiles);
    }

    {
        currentZoomLevelIdentifier = targetZoomLevelIdentifier;
    }

    if (lastVisibleTilesHash != visibleTileHash) {
        lastVisibleTilesHash = visibleTileHash;
        onVisibleTilesChanged(layers, keepZoomLevelOffset);
    }
    currentViewBounds = visibleBoundsLayer;
}

template<class T, class L, class R>
void Tiled2dMapSource<T, L, R>::onVisibleTilesChanged(const std::vector<VisibleTilesLayer> &pyramid, int keepZoomLevelOffset) {

    currentVisibleTiles.clear();

    std::vector<PrioritizedTiled2dMapTileInfo> toAdd;

    // make sure all tiles on the current zoom level are scheduled to load (as well as those from the level we always want to keep)
    for (const auto &layer: pyramid) {
        if ((layer.targetZoomLevelOffset <= 0 && layer.targetZoomLevelOffset >= -zoomInfo.numDrawPreviousLayers) || layer.targetZoomLevelOffset == keepZoomLevelOffset){
            for (auto const &tileInfo: layer.visibleTiles) {
                currentVisibleTiles.insert(tileInfo.tileInfo);

                size_t currentTilesCount = currentTiles.count(tileInfo.tileInfo);
                size_t currentlyLoadingCount = currentlyLoading.count(tileInfo.tileInfo);
                size_t notFoundCount = notFoundTiles.count(tileInfo.tileInfo);

                if (currentTilesCount == 0 && currentlyLoadingCount == 0 && notFoundCount == 0) {
                    toAdd.push_back(tileInfo);
                }
            }
        }
    }

    currentPyramid = pyramid;
    currentKeepZoomLevelOffset = keepZoomLevelOffset;

    // we only remove tiles that are not visible anymore directly
    // tile from upper zoom levels will be removed as soon as the correct tiles are loaded if mask tiles is enabled
    std::vector<Tiled2dMapTileInfo> toRemove;

    int currentZoomLevelIdentifier = this->currentZoomLevelIdentifier;
    bool onlyCurrent = !zoomInfo.maskTile && zoomInfo.numDrawPreviousLayers == 0;
    for (const auto &[tileInfo, tileWrapper] : currentTiles) {
        bool found = false;

        if ((!onlyCurrent && tileInfo.zoomIdentifier <= currentZoomLevelIdentifier)
            || (onlyCurrent && tileInfo.zoomIdentifier == currentZoomLevelIdentifier)
            || tileInfo.zoomIdentifier == (currentZoomLevelIdentifier + keepZoomLevelOffset)) {
            for (const auto &layer: pyramid) {
                for (auto const &tile: layer.visibleTiles) {
                    if (tileInfo == tile.tileInfo) {
                        found = true;
                        break;
                    }
                }

                if(found) { break; }
            }
        }

        if (!found) {
            toRemove.push_back(tileInfo);
        }
    }

    for (const auto &removedTile : toRemove) {
        gpc_free_polygon(&currentTiles.at(removedTile).tilePolygon);
        currentTiles.erase(removedTile);
        currentlyLoading.erase(removedTile);

        readyTiles.erase(removedTile);

        if (errorManager)
            errorManager->removeError(
                    layerConfig->getTileUrl(removedTile.x, removedTile.y, removedTile.t, removedTile.zoomIdentifier));
    }

    for (auto it = currentlyLoading.begin(); it != currentlyLoading.end();) {
        bool found = false;
        if (it->first.zoomIdentifier <= currentZoomLevelIdentifier) {
            for (const auto &layer: pyramid) {
                for (auto const &tile: layer.visibleTiles) {
                    if (it->first == tile.tileInfo) {
                        found = true;
                        break;
                    }
                }

                if(found) { break; }
            }
        }

       if(!found) {
          cancelLoad(it->first, it->second);
          it = currentlyLoading.erase(it);
       }
       else
          it++;
    }

    for (auto &[loaderIndex, errors]: errorTiles) {
        for (auto it = errors.begin(); it != errors.end();) {
            bool found = false;
            for (const auto &layer: pyramid) {
                auto visibleTile = layer.visibleTiles.find({it->first, 0});
                if (visibleTile == layer.visibleTiles.end()) {
                    found = true;
                    break;
                }
            }
            if (found) {
                if (errorManager) errorManager->removeError(layerConfig->getTileUrl(it->first.x, it->first.y, it->first.t, it->first.zoomIdentifier));
                it = errors.erase(it);
            } else {
                ++it;
            }
        }
    }

    std::sort(toAdd.begin(), toAdd.end());

    for (const auto &addedTile : toAdd) {
        performLoadingTask(addedTile.tileInfo, 0);
    }
    //if we removed tiles, we potentially need to update the tilemasks - also if no new tile is loaded
    updateTileMasks();
    
    notifyTilesUpdates();
}

template<class T, class L, class R>
void Tiled2dMapSource<T, L, R>::performLoadingTask(Tiled2dMapTileInfo tile, size_t loaderIndex) {
    if (currentlyLoading.count(tile) != 0) return;

    if (currentVisibleTiles.count(tile) == 0) {
        errorTiles[loaderIndex].erase(tile);
        return;
    };

    std::weak_ptr<Tiled2dMapSource> weakSelfPtr = std::dynamic_pointer_cast<Tiled2dMapSource>(shared_from_this());
    auto weakActor = WeakActor<Tiled2dMapSource>(mailbox, std::static_pointer_cast<Tiled2dMapSource>(shared_from_this()));

    currentlyLoading.insert({tile, loaderIndex});
    readyTiles.erase(tile);

    loadDataAsync(tile, loaderIndex).then([weakActor, loaderIndex, tile, weakSelfPtr](::djinni::Future<L> result) {

        auto strongSelf = weakSelfPtr.lock();
        if (strongSelf) {
            auto res = result.get();
            if (res->status == LoaderStatus::OK) {
                if (strongSelf->hasExpensivePostLoadingTask()) {
                    auto strongScheduler = strongSelf->scheduler.lock();
                    if (strongScheduler) {
                        strongScheduler->addTask(std::make_shared<LambdaTask>(
                                TaskConfig("postLoadingTask", 0.0, TaskPriority::NORMAL, ExecutionEnvironment::COMPUTATION),
                                [tile, loaderIndex, weakSelfPtr, weakActor, res] {
                                    auto strongSelf = weakSelfPtr.lock();
                                    if (strongSelf) {
                                        weakActor.message(&Tiled2dMapSource::didLoad, tile, loaderIndex,
                                                          strongSelf->postLoadingTask(res, tile));
                                    }
                                }));
                    }
                } else {
                    weakActor.message(&Tiled2dMapSource::didLoad, tile, loaderIndex, strongSelf->postLoadingTask(res, tile));
                }
            } else {
                weakActor.message(&Tiled2dMapSource::didFailToLoad, tile, loaderIndex, res->status, res->errorCode);
            }
        }
    });
}

template<class T, class L, class R>
void Tiled2dMapSource<T, L, R>::didLoad(Tiled2dMapTileInfo tile, size_t loaderIndex, const R &result) {
    currentlyLoading.erase(tile);

    const bool isVisible = currentVisibleTiles.count(tile);
    if (!isVisible) {
        errorTiles[loaderIndex].erase(tile);
        return;
    }

    auto errorManager = this->errorManager;

    if (errorManager) {
        errorManager->removeError(layerConfig->getTileUrl(tile.x, tile.y, tile.t, tile.zoomIdentifier));
    }

    auto bounds = tile.bounds;
    PolygonCoord mask({ bounds.topLeft,
        Coord(bounds.topLeft.systemIdentifier, bounds.bottomRight.x, bounds.topLeft.y, 0),
        bounds.bottomRight,
        Coord(bounds.topLeft.systemIdentifier, bounds.topLeft.x, bounds.bottomRight.y, 0),
        bounds.topLeft }, {});

    gpc_polygon tilePolygon;
    gpc_set_polygon({mask}, &tilePolygon);

    currentTiles.insert({tile, TileWrapper<R>(result, std::vector<::PolygonCoord>{  }, mask, tilePolygon)});

    errorTiles[loaderIndex].erase(tile);

    updateTileMasks();

    notifyTilesUpdates();
}

template<class T, class L, class R>
void Tiled2dMapSource<T, L, R>::didFailToLoad(Tiled2dMapTileInfo tile, size_t loaderIndex, const LoaderStatus &status, const std::optional<std::string> &errorCode) {
    currentlyLoading.erase(tile);

    const bool isVisible = currentVisibleTiles.count(tile);
    if (!isVisible) {
        errorTiles[loaderIndex].erase(tile);
        return;
    }

    auto errorManager = this->errorManager;

    switch (status) {
        case LoaderStatus::OK: {
            assert(false);
            break;
        }
        case LoaderStatus::NOOP: {
            errorTiles[loaderIndex].erase(tile);

            auto newLoaderIndex = loaderIndex + 1;
            performLoadingTask(tile, newLoaderIndex);

            break;
        }
        case LoaderStatus::ERROR_400:
        case LoaderStatus::ERROR_404: {
            notFoundTiles.insert(tile);

            errorTiles[loaderIndex].erase(tile);

            if (errorManager) {
                errorManager->addTiledLayerError(TiledLayerError(status,
                                                                 errorCode,
                                                                 layerName,
                                                                 layerConfig->getTileUrl(tile.x, tile.y, tile.t,tile.zoomIdentifier),
                                                                 false,
                                                                 tile.bounds));
            }
            break;
        }

        case LoaderStatus::ERROR_TIMEOUT:
        case LoaderStatus::ERROR_OTHER:
        case LoaderStatus::ERROR_NETWORK: {
            const auto now = DateHelper::currentTimeMillis();
            int64_t delay = 0;
            if (errorTiles[loaderIndex].count(tile) != 0) {
                errorTiles[loaderIndex].at(tile).lastLoad = now;
                errorTiles[loaderIndex].at(tile).delay = std::min(2 * errorTiles[loaderIndex].at(tile).delay, MAX_WAIT_TIME);
            } else {
                errorTiles[loaderIndex][tile] = {now, MIN_WAIT_TIME};
            }
            delay = errorTiles[loaderIndex].at(tile).delay;

            if (errorManager) {
                errorManager->addTiledLayerError(TiledLayerError(status,
                                                                 errorCode,
                                                                 layerConfig->getLayerName(),
                                                                 layerConfig->getTileUrl(tile.x, tile.y, tile.t, tile.zoomIdentifier),
                                                                 true,
                                                                 tile.bounds));
            }

            if (!nextDelayTaskExecution || nextDelayTaskExecution > now + delay ) {
                nextDelayTaskExecution = now + delay;

                auto taskIdentifier = "Tiled2dMapSource_loadingErrorTask";

                auto strongScheduler = scheduler.lock();
                if (strongScheduler) {
                    auto weakActor = WeakActor<Tiled2dMapSource>(mailbox, std::dynamic_pointer_cast<Tiled2dMapSource>(shared_from_this()));
                    strongScheduler->addTask(std::make_shared<LambdaTask>(TaskConfig(taskIdentifier, delay, TaskPriority::NORMAL, ExecutionEnvironment::IO), [weakActor] {
                        weakActor.message(&Tiled2dMapSource::performDelayedTasks);
                    }));
                }
            }
            break;
        }
    }

    updateTileMasks();

    notifyTilesUpdates();
}


template<class T, class L, class R>
void Tiled2dMapSource<T, L, R>::performDelayedTasks() {
    nextDelayTaskExecution = std::nullopt;

    const auto now = DateHelper::currentTimeMillis();
    long long minDelay = std::numeric_limits<long long>::max();

    std::vector<std::pair<int, Tiled2dMapTileInfo>> toLoad;

    for (auto &[loaderIndex, errors]: errorTiles) {
        for(auto &[tile, errorInfo]: errors) {
            if (errorInfo.lastLoad + errorInfo.delay >= now) {
                toLoad.push_back({loaderIndex, tile});
            } else {
                minDelay = std::min(minDelay, errorInfo.delay);
            }
        }
    }

    for (auto &[loaderIndex, tile]: toLoad) {
        performLoadingTask(tile, loaderIndex);
    }

    if (minDelay != std::numeric_limits<long long>::max()) {
        nextDelayTaskExecution = now + minDelay;

        auto taskIdentifier = "Tiled2dMapSource_loadingErrorTask";

        auto strongScheduler = scheduler.lock();
        if (strongScheduler) {
            auto weakActor = WeakActor<Tiled2dMapSource>(mailbox, std::dynamic_pointer_cast<Tiled2dMapSource>(shared_from_this()));
            strongScheduler->addTask(std::make_shared<LambdaTask>(TaskConfig(taskIdentifier, minDelay, TaskPriority::NORMAL, ExecutionEnvironment::IO), [weakActor] {
                weakActor.message(&Tiled2dMapSource::performDelayedTasks);
            }));
        }
    }
}

template<class T, class L, class R>
void Tiled2dMapSource<T, L, R>::updateTileMasks() {

    if (!zoomInfo.maskTile) {
        return;
    }

    if (currentTiles.empty() && outdatedTiles.empty()) {
        return;
    }

    std::vector<Tiled2dMapTileInfo> tilesToRemove;

    int currentZoomLevelIdentifier = this->currentZoomLevelIdentifier;

    gpc_polygon currentTileMask;
    bool freeCurrent = false;
    currentTileMask.num_contours = 0;
    bool isFirst = true;

    gpc_polygon currentViewBoundsPolygon;
    gpc_set_polygon({PolygonCoord({
        currentViewBounds.topLeft,
        Coord(currentViewBounds.topLeft.systemIdentifier, currentViewBounds.bottomRight.x,
              currentViewBounds.topLeft.y, 0),
        currentViewBounds.bottomRight,
        Coord(currentViewBounds.topLeft.systemIdentifier, currentViewBounds.topLeft.x,
              currentViewBounds.bottomRight.y, 0),
        currentViewBounds.topLeft
    }, {})}, &currentViewBoundsPolygon);

    bool completeViewBoundsDrawn = false;

    for (auto it = currentTiles.rbegin(); it != currentTiles.rend(); it++){
        auto &[tileInfo, tileWrapper] = *it;

        tileWrapper.state = TileState::VISIBLE;

        if (readyTiles.count(tileInfo) == 0) {
            tileWrapper.state = TileState::IN_SETUP;
            continue;
        }

        if (tileInfo.zoomIdentifier != currentZoomLevelIdentifier) {

            if (currentTileMask.num_contours != 0) {
                if (!completeViewBoundsDrawn) {
                    gpc_polygon diff;
                    gpc_polygon_clip(GPC_DIFF, &currentViewBoundsPolygon, &currentTileMask, &diff);

                    if (diff.num_contours == 0) {
                        completeViewBoundsDrawn = true;
                    }

                    gpc_free_polygon(&diff);
                }
            }

            if (completeViewBoundsDrawn) {
                tileWrapper.state = TileState::CACHED;
                continue;
            }

            gpc_polygon polygonDiff;
            bool freePolygonDiff = false;
            if (currentTileMask.num_contours != 0) {
                freePolygonDiff = true;
                gpc_polygon_clip(GPC_DIFF, &tileWrapper.tilePolygon, &currentTileMask, &polygonDiff);
            } else {
                polygonDiff = tileWrapper.tilePolygon;
            }

            if (polygonDiff.contour == NULL) {
                tileWrapper.state = TileState::CACHED;
                if (freePolygonDiff) {
                    gpc_free_polygon(&polygonDiff);
                }
                continue;
            } else {
                gpc_polygon resultingMask;

                gpc_polygon_clip(GPC_INT, &polygonDiff, &currentViewBoundsPolygon, &resultingMask);

                if (resultingMask.contour == NULL) {
                    tileWrapper.state = TileState::CACHED;
                    if (freePolygonDiff) {
                        gpc_free_polygon(&polygonDiff);
                    }
                    continue;
                } else {
                    tileWrapper.masks = gpc_get_polygon_coord(&polygonDiff, tileInfo.bounds.topLeft.systemIdentifier);
                }

                gpc_free_polygon(&resultingMask);
            }

            if (freePolygonDiff) {
                gpc_free_polygon(&polygonDiff);
            }
        } else {
            tileWrapper.masks = { tileWrapper.tileBounds };
        }

        // add tileBounds to currentTileMask
        if (tileWrapper.state == TileState::VISIBLE) {
            if (isFirst) {
                gpc_set_polygon({ tileWrapper.tileBounds }, &currentTileMask);
                isFirst = false;
            } else {
                gpc_polygon result;
                gpc_polygon_clip(GPC_UNION, &currentTileMask, &tileWrapper.tilePolygon, &result);
                gpc_free_polygon(&currentTileMask);
                currentTileMask = result;
            }

            freeCurrent = true;
        }
    }

    if(freeCurrent) {
        gpc_free_polygon(&currentTileMask);
    }
    gpc_free_polygon(&currentViewBoundsPolygon);
}

template<class T, class L, class R>
void Tiled2dMapSource<T, L, R>::setTileReady(const Tiled2dMapVersionedTileInfo &tile) {
    bool needsUpdate = false;
    
    if (readyTiles.count(tile.tileInfo) == 0) {
        if (currentTiles.count(tile.tileInfo) != 0){
            readyTiles.insert(tile.tileInfo);
            outdatedTiles.erase(tile.tileInfo);
            needsUpdate = true;
        }
    }
    
    if (!needsUpdate) { return; }
    
    updateTileMasks();
    
    notifyTilesUpdates();
}

template<class T, class L, class R>
void Tiled2dMapSource<T, L, R>::setTilesReady(const std::vector<Tiled2dMapVersionedTileInfo> &tiles) {
    bool needsUpdate = false;
    
    for (auto const &tile: tiles) {
        if (readyTiles.count(tile.tileInfo) == 0 || outdatedTiles.count(tile.tileInfo) > 0) {
            if (currentTiles.count(tile.tileInfo) != 0){
                readyTiles.insert(tile.tileInfo);
                outdatedTiles.erase(tile.tileInfo);
                needsUpdate = true;
            }
        }
    }
    
    if (!needsUpdate) { return; }

    updateTileMasks();
    notifyTilesUpdates();
}

template<class T, class L, class R>
RectCoord Tiled2dMapSource<T, L, R>::getCurrentViewBounds() {
    return currentViewBounds;
}

template<class T, class L, class R>
void Tiled2dMapSource<T, L, R>::setMinZoomLevelIdentifier(std::optional<int32_t> value) {
    minZoomLevelIdentifier = value;
}

template<class T, class L, class R>
void Tiled2dMapSource<T, L, R>::setMaxZoomLevelIdentifier(std::optional<int32_t> value) {
    maxZoomLevelIdentifier = value;
}

template<class T, class L, class R>
std::optional<int32_t> Tiled2dMapSource<T, L, R>::getMinZoomLevelIdentifier() {
    return minZoomLevelIdentifier;
}

template<class T, class L, class R>
std::optional<int32_t> Tiled2dMapSource<T, L, R>::getMaxZoomLevelIdentifier() {
    return maxZoomLevelIdentifier;
}

template<class T, class L, class R>
void Tiled2dMapSource<T, L, R>::pause() {
    isPaused = true;
}

template<class T, class L, class R>
void Tiled2dMapSource<T, L, R>::resume() {
    isPaused = false;
}

template<class T, class L, class R>
::LayerReadyState Tiled2dMapSource<T, L, R>::isReadyToRenderOffscreen() {
    if (notFoundTiles.size() > 0) {
        return LayerReadyState::ERROR;
    }

    for (auto const &[index, errors]: errorTiles) {
        if (errors.size() > 0) {
            return LayerReadyState::ERROR;
        }
    }

    if (!currentlyLoading.empty()) {
        return LayerReadyState::NOT_READY;
    }

    for (const auto& visible : currentVisibleTiles) {
        if (currentTiles.count(visible) == 0) {
            return LayerReadyState::NOT_READY;
        }
        if (readyTiles.count(visible) == 0) {
            return LayerReadyState::NOT_READY;
        }
    }

    return LayerReadyState::READY;
}

template<class T, class L, class R>
void Tiled2dMapSource<T, L, R>::setErrorManager(const std::shared_ptr<::ErrorManager> &errorManager) {
    this->errorManager = errorManager;
}

template<class T, class L, class R>
void Tiled2dMapSource<T, L, R>::forceReload() {

    //set delay to 0 for all error tiles
    for (auto &[loaderIndex, errors]: errorTiles) {
        for(auto &[tile, errorInfo]: errors) {
            errorInfo.delay = 1;
            performLoadingTask(tile, loaderIndex);
        }
    }

}

template<class T, class L, class R>
void Tiled2dMapSource<T, L, R>::reloadTiles() {
    outdatedTiles.clear();
    outdatedTiles.insert(currentTiles.begin(), currentTiles.end());
    
    currentTiles.clear();
    readyTiles.clear();

    for (auto it = currentlyLoading.begin(); it != currentlyLoading.end(); ++it ) {
        cancelLoad(it->first, it->second);
    }
    currentlyLoading.clear();
    errorTiles.clear();

    lastVisibleTilesHash = -1;
    onVisibleTilesChanged(currentPyramid, currentKeepZoomLevelOffset);
}

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
#include "LoaderStatus.h"
#include <algorithm>

template <class T, class L>
Tiled2dMapSource<T, L>::Tiled2dMapSource(const MapConfig &mapConfig, const std::shared_ptr<Tiled2dMapLayerConfig> &layerConfig,
                                         const std::shared_ptr<CoordinateConversionHelperInterface> &conversionHelper,
                                         const std::shared_ptr<SchedulerInterface> &scheduler,
                                         const std::shared_ptr<Tiled2dMapSourceListenerInterface> &listener)
    : mapConfig(mapConfig)
    , layerConfig(layerConfig)
    , conversionHelper(conversionHelper)
    , scheduler(scheduler)
    , listener(listener)
    , zoomLevelInfos(layerConfig->getZoomLevelInfos())
    , zoomInfo(layerConfig->getZoomInfo())
    , layerSystemId(layerConfig->getCoordinateSystemIdentifier())
    , dispatchedTasks(0) {

    std::sort(zoomLevelInfos.begin(), zoomLevelInfos.end(),
              [](const Tiled2dMapZoomLevelInfo &a, const Tiled2dMapZoomLevelInfo &b) -> bool { return a.zoom > b.zoom; });
}

template <class T, class L> void Tiled2dMapSource<T, L>::onVisibleBoundsChanged(const ::RectCoord &visibleBounds, double zoom) {
    scheduler->addTask(std::make_shared<LambdaTask>(
        TaskConfig("Tiled2dMapSource_Update", 0, TaskPriority::NORMAL, ExecutionEnvironment::COMPUTATION),
        [=] { updateCurrentTileset(visibleBounds, zoom); }));
}

template <class T, class L> void Tiled2dMapSource<T, L>::updateCurrentTileset(const RectCoord &visibleBounds, double zoom) {
    std::unordered_set<PrioritizedTiled2dMapTileInfo> visibleTiles;

    RectCoord visibleBoundsLayer = conversionHelper->convertRect(layerSystemId, visibleBounds);

    double centerVisibleX = visibleBoundsLayer.topLeft.x + 0.5 * (visibleBoundsLayer.bottomRight.x - visibleBoundsLayer.topLeft.x);
    double centerVisibleY = visibleBoundsLayer.topLeft.y + 0.5 * (visibleBoundsLayer.bottomRight.y - visibleBoundsLayer.topLeft.y);

    size_t numZoomLevels = zoomLevelInfos.size();
    int targetZoomLayer = 0;
    for (int i = 0; i < numZoomLevels; i++) {
        const Tiled2dMapZoomLevelInfo &zoomLevelInfo = zoomLevelInfos.at(i);
        if (zoomInfo.zoomLevelScaleFactor * zoomLevelInfo.zoom < zoom || i + 1 == numZoomLevels) {
            targetZoomLayer = i;
            break;
        }
    }
    int startZoomLayer = std::max(targetZoomLayer - zoomInfo.numDrawPreviousLayers, 0);

    int zoomInd = 0;
    int zoomPriorityRange = 20;
    for (int i = startZoomLayer; i <= targetZoomLayer; i++) {
        const Tiled2dMapZoomLevelInfo &zoomLevelInfo = zoomLevelInfos.at(i);

        double tileWidth = zoomLevelInfo.tileWidthLayerSystemUnits;

        RectCoord layerBounds = zoomLevelInfo.bounds;
        layerBounds = conversionHelper->convertRect(layerSystemId, layerBounds);

        bool leftToRight = layerBounds.topLeft.x < layerBounds.bottomRight.x;
        bool topToBottom = layerBounds.topLeft.y < layerBounds.bottomRight.y;
        double tileWidthAdj = leftToRight ? tileWidth : -tileWidth;
        double tileHeightAdj = topToBottom ? tileWidth : -tileWidth;

        double visibleLeft = visibleBoundsLayer.topLeft.x;
        double visibleRight = visibleBoundsLayer.bottomRight.x;
        double visibleWidth = std::abs(visibleLeft - visibleRight);
        double boundsLeft = layerBounds.topLeft.x;
        int startTileLeft =
            std::floor(std::max(leftToRight ? (visibleLeft - boundsLeft) : (boundsLeft - visibleLeft), 0.0) / tileWidth);
        int maxTileLeft = std::ceil(visibleWidth / tileWidth) + startTileLeft;
        double visibleTop = visibleBoundsLayer.topLeft.y;
        double visibleBottom = visibleBoundsLayer.bottomRight.y;
        double visibleHeight = std::abs(visibleTop - visibleBottom);
        double boundsTop = layerBounds.topLeft.y;
        int startTileTop = std::floor(std::max(topToBottom ? (visibleTop - boundsTop) : (boundsTop - visibleTop), 0.0) / tileWidth);
        int maxTileTop = std::ceil(visibleHeight / tileWidth) + startTileTop;

        double maxDisCenterX = visibleWidth * 0.5 + tileWidth;
        double maxDisCenterY = visibleHeight * 0.5 + tileWidth;
        double maxDisCenter = std::sqrt(maxDisCenterX * maxDisCenterX + maxDisCenterY * maxDisCenterY);

        for (int x = startTileLeft; x <= maxTileLeft && x < zoomLevelInfo.numTilesX; x++) {
            for (int y = startTileTop; y <= maxTileTop && y < zoomLevelInfo.numTilesY; y++) {
                Coord tileTopLeft = Coord(layerSystemId, x * tileWidthAdj + boundsLeft, y * tileHeightAdj + boundsTop, 0);
                Coord tileBottomRight = Coord(layerSystemId, tileTopLeft.x + tileWidthAdj, tileTopLeft.y + tileHeightAdj, 0);
                RectCoord rect(tileTopLeft, tileBottomRight);

                double tileCenterX = tileTopLeft.x + 0.5f * (tileBottomRight.x - tileTopLeft.x);
                double tileCenterY = tileTopLeft.y + 0.5f * (tileBottomRight.y - tileTopLeft.y);
                double tileCenterDis =
                    std::sqrt(std::pow(tileCenterX - centerVisibleX, 2.0) + std::pow(tileCenterY - centerVisibleY, 2.0));

                visibleTiles.insert(PrioritizedTiled2dMapTileInfo(
                    Tiled2dMapTileInfo(rect, x, y, zoomLevelInfo.zoomLevelIdentifier, i),
                    std::ceil((tileCenterDis / maxDisCenter) * zoomPriorityRange) + zoomInd * zoomPriorityRange));
            }
        }

        zoomInd++;
    }

    onVisibleTilesChanged(visibleTiles);
}

template <class T, class L>
void Tiled2dMapSource<T, L>::onVisibleTilesChanged(const std::unordered_set<PrioritizedTiled2dMapTileInfo> &visibleTiles) {
    std::lock_guard<std::recursive_mutex> lock(tilesMutex);
    currentVisibleTiles.clear();

    std::unordered_set<PrioritizedTiled2dMapTileInfo> toAdd;
    for (const auto &tileInfo : visibleTiles) {
        currentVisibleTiles.insert(tileInfo.tileInfo);

        if (currentTiles.count(tileInfo.tileInfo) == 0 &&
            currentlyLoading.count(tileInfo.tileInfo) == 0) {
            for (auto it = loadingQueue.begin(); it != loadingQueue.end(); it++) {
                if (it->tileInfo == tileInfo.tileInfo) {
                    loadingQueue.erase(it);
                    break;
                }
            }

                toAdd.insert(tileInfo);
        }
    }

    std::unordered_set<Tiled2dMapTileInfo> toRemove;
    for (const auto &tileEntry : currentTiles) {
        bool found = false;
        for (const auto &tile : visibleTiles) {
            if (tileEntry.first == tile.tileInfo) {
                found = true;
                break;
            }
        }
        if (!found) {
            toRemove.insert(tileEntry.first);
        }
    }

    for (const auto &removedTile : toRemove) {
        currentTiles.erase(removedTile);
    }

    for (auto it = loadingQueue.begin(); it != loadingQueue.end();) {
        if (visibleTiles.count(*it) == 0) {
            it = loadingQueue.erase(it);
        } else {
            ++it;
        }
    }

    for (auto it = errorTiles.begin(); it != errorTiles.end();) {
        if (visibleTiles.count({it->first, 0}) == 0) {
            it = errorTiles.erase(it);
        } else {
            ++it;
        }
    }

    for (const auto &addedTile : toAdd) {
        if (loadingQueue.count(addedTile) == 0 &&
            errorTiles.count(addedTile.tileInfo) == 0) {
            loadingQueue.insert(addedTile);
        }
    }

    size_t errorTilesCount = errorTiles.size();

    size_t totalOutstandingTasks = errorTilesCount + loadingQueue.size();
    size_t dispatchedTaskCount = dispatchedTasks.load();
    size_t tasksToAdd = totalOutstandingTasks > dispatchedTaskCount ? totalOutstandingTasks - dispatchedTaskCount : 0;
    for (int taskCount = 0; taskCount < tasksToAdd; taskCount++) {
        auto taskIdentifier = "Tiled2dMapSource_loadingTask" + std::to_string(taskCount);
        scheduler->addTask(std::make_shared<LambdaTask>(
            TaskConfig(taskIdentifier, 0, TaskPriority::NORMAL, ExecutionEnvironment::IO), [=] { performLoadingTask(); }));
        dispatchedTasks++;
    }

    listener->onTilesUpdated();
}

template <class T, class L> std::optional<Tiled2dMapTileInfo> Tiled2dMapSource<T, L>::dequeueLoadingTask() {
    std::lock_guard<std::recursive_mutex> lock(tilesMutex);

    dispatchedTasks--;

    if (loadingQueue.empty()) {
        for (auto const &errorTile : errorTiles) {
            if (errorTile.second.lastLoad + errorTile.second.delay <= DateHelper::currentTimeMillis() &&
                currentlyLoading.count(errorTile.first) == 0) {
                currentlyLoading.insert(errorTile.first);
                return errorTile.first;
            }
        }
        return std::nullopt;
    }

    auto tile = loadingQueue.begin();
    auto tileInfo = tile->tileInfo;
    loadingQueue.erase(tile);

    currentlyLoading.insert(tileInfo);

    return tileInfo;
}

template <class T, class L> void Tiled2dMapSource<T, L>::performLoadingTask() {
    if (auto tile = dequeueLoadingTask()) {
        auto loaderResult = loadTile(*tile);

        std::lock_guard<std::recursive_mutex> lock(tilesMutex);

        LoaderStatus status = loaderResult.status;

        switch (status) {
        case LoaderStatus::OK: {
            errorTiles.erase(*tile);

            if (currentVisibleTiles.count(*tile)) {
                currentTiles[*tile] = loaderResult.data;
            }
            break;
        }
        case LoaderStatus::ERROR_400:
        case LoaderStatus::ERROR_404: {
            notFoundTiles.insert(*tile);
            break;
        }

        case LoaderStatus::ERROR_TIMEOUT:
        case LoaderStatus::ERROR_OTHER:
        case LoaderStatus::ERROR_NETWORK: {
            if (errorTiles.count(*tile) != 0) {
                errorTiles[*tile].lastLoad = DateHelper::currentTimeMillis();
                errorTiles[*tile].delay = std::min(2 * errorTiles[*tile].delay, MAX_WAIT_TIME);
            } else {
                errorTiles[*tile] = {DateHelper::currentTimeMillis(), MIN_WAIT_TIME};
            }
            auto delay = errorTiles[*tile].delay;
            auto taskIdentifier = "Tiled2dMapSource_loadingErrorTask";
            dispatchedTasks++;
            scheduler->addTask(std::make_shared<LambdaTask>(
                TaskConfig(taskIdentifier, delay, TaskPriority::NORMAL, ExecutionEnvironment::IO), [=] { performLoadingTask(); }));
            break;
        }
        }

        currentlyLoading.erase(*tile);

        listener->onTilesUpdated();
    }
}

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
                                         const std::shared_ptr<Tiled2dMapSourceListenerInterface> &listener,
                                         float screenDensityPpi,
                                         size_t loaderCount)
    : mapConfig(mapConfig)
    , layerConfig(layerConfig)
    , conversionHelper(conversionHelper)
    , scheduler(scheduler)
    , listener(listener)
    , zoomLevelInfos(layerConfig->getZoomLevelInfos())
    , zoomInfo(layerConfig->getZoomInfo())
    , layerSystemId(layerConfig->getCoordinateSystemIdentifier())
    , dispatchedTasks(0)
    , isPaused(false)
    , screenDensityPpi(screenDensityPpi) {
    std::sort(zoomLevelInfos.begin(), zoomLevelInfos.end(),
              [](const Tiled2dMapZoomLevelInfo &a, const Tiled2dMapZoomLevelInfo &b) -> bool { return a.zoom > b.zoom; });

    for (size_t i = 0; i != loaderCount; i++) {
        loadingQueues[i] = {};
        dispatchedTasks[i] = 0;
    }
}

template<class T, class L, class R>
void Tiled2dMapSource<T, L, R>::onVisibleBoundsChanged(const ::RectCoord &visibleBounds, int curT, double zoom) {
    if (isPaused) {
        return;
    }
    {
        std::lock_guard<std::recursive_mutex> updateLock(updateMutex);
        updateBounds = visibleBounds;
        updateT = curT;
        updateZoom = zoom;
    }
    if (updateFlag.test_and_set()) {
        return;
    }

    pendingUpdates++;
    std::weak_ptr<Tiled2dMapSource> weakSelfPtr = std::dynamic_pointer_cast<Tiled2dMapSource>(shared_from_this());
    scheduler->addTask(std::make_shared<LambdaTask>(
            TaskConfig("Tiled2dMapSource_Update", 0, TaskPriority::NORMAL, ExecutionEnvironment::IO),
            [weakSelfPtr] {
                auto selfPtr = weakSelfPtr.lock();
                if (selfPtr) {
                    std::lock_guard<std::recursive_mutex> updateLock(selfPtr->updateTilesetMutex);
                    std::optional<RectCoord> bounds;
                    std::optional<int> curT;
                    std::optional<double> zoom;
                    {
                        std::lock_guard<std::recursive_mutex> updateLock(selfPtr->updateMutex);
                        bounds = selfPtr->updateBounds;
                        curT = selfPtr->updateT;
                        zoom = selfPtr->updateZoom;
                    }
                    selfPtr->updateFlag.clear();
                    if (bounds.has_value() && zoom.has_value() && curT.has_value()) {
                        selfPtr->updateCurrentTileset(*bounds, *curT, *zoom);
                    }
                    selfPtr->pendingUpdates--;
                }
            }));
}

template<class T, class L, class R>
bool Tiled2dMapSource<T, L, R>::isTileVisible(const Tiled2dMapTileInfo &tileInfo) {
    std::lock_guard<std::recursive_mutex> lock(currentVisibleTilesMutex);
    return currentVisibleTiles.count(tileInfo) > 0;
}

template<class T, class L, class R>
void Tiled2dMapSource<T, L, R>::updateCurrentTileset(const RectCoord &visibleBounds, int curT, double zoom) {
    std::vector<PrioritizedTiled2dMapTileInfo> visibleTilesVec;

    RectCoord visibleBoundsLayer = conversionHelper->convertRect(layerSystemId, visibleBounds);

    double centerVisibleX = visibleBoundsLayer.topLeft.x + 0.5 * (visibleBoundsLayer.bottomRight.x - visibleBoundsLayer.topLeft.x);
    double centerVisibleY = visibleBoundsLayer.topLeft.y + 0.5 * (visibleBoundsLayer.bottomRight.y - visibleBoundsLayer.topLeft.y);

    size_t numZoomLevels = zoomLevelInfos.size();
    int targetZoomLayer = -1;
    // Each pixel is assumed to be 0.28mm – https://gis.stackexchange.com/a/315989
    const float screenScaleFactor = zoomInfo.adaptScaleToScreen ? screenDensityPpi / (0.0254 / 0.00028) : 1.0;
    for (int i = 0; i < numZoomLevels; i++) {
        const Tiled2dMapZoomLevelInfo &zoomLevelInfo = zoomLevelInfos.at(i);
        if (zoomInfo.zoomLevelScaleFactor * screenScaleFactor * zoomLevelInfo.zoom < zoom) {
            targetZoomLayer = std::max(i - 1, 0);
            break;
        }
    }
    if (targetZoomLayer < 0) {
        targetZoomLayer = (int) numZoomLevels - 1;
    }
    int targetZoomLevelIdentifier = zoomLevelInfos.at(targetZoomLayer).zoomLevelIdentifier;
    int startZoomLayer = 0;
    int endZoomLevel = std::min((int) numZoomLevels - 1, targetZoomLayer + 2);

    int zoomInd = 0;
    int tPriorityRange = 1000 * zoomLevelInfos.at(0).numTilesT;
    int zPriorityRange = 100;
    int zoomPriorityRange = 100; // TODO: Was ist der unterschied zu

    std::vector<VisibleTilesLayer> layers;

    for (int i = startZoomLayer; i <= endZoomLevel; i++) {
        const Tiled2dMapZoomLevelInfo &zoomLevelInfo = zoomLevelInfos.at(i);

        if (minZoomLevelIdentifier.has_value() && zoomLevelInfo.zoomLevelIdentifier < minZoomLevelIdentifier) {
            continue;
        }
        if (maxZoomLevelIdentifier.has_value() && zoomLevelInfo.zoomLevelIdentifier > maxZoomLevelIdentifier) {
            continue;
        }

        VisibleTilesLayer curVisibleTiles(i - targetZoomLayer, i);
        std::vector<PrioritizedTiled2dMapTileInfo> curVisibleTilesVec;

        const double tileWidth = zoomLevelInfo.tileWidthLayerSystemUnits;

        RectCoord layerBounds = zoomLevelInfo.bounds;
        layerBounds = conversionHelper->convertRect(layerSystemId, layerBounds);

        const bool leftToRight = layerBounds.topLeft.x < layerBounds.bottomRight.x;
        const bool topToBottom = layerBounds.topLeft.y < layerBounds.bottomRight.y;
        const double tileWidthAdj = leftToRight ? tileWidth : -tileWidth;
        const double tileHeightAdj = topToBottom ? tileWidth : -tileWidth;

        const double visibleLeft = visibleBoundsLayer.topLeft.x;
        const double visibleRight = visibleBoundsLayer.bottomRight.x;
        const double visibleWidth = std::abs(visibleLeft - visibleRight);
        const double boundsLeft = layerBounds.topLeft.x;
        const int startTileLeft =
                std::floor(std::max(leftToRight ? (visibleLeft - boundsLeft) : (boundsLeft - visibleLeft), 0.0) / tileWidth);
        const int maxTileLeft = std::floor(
                std::max(leftToRight ? (visibleRight - boundsLeft) : (boundsLeft - visibleRight), 0.0) / tileWidth);
        const double visibleTop = visibleBoundsLayer.topLeft.y;
        const double visibleBottom = visibleBoundsLayer.bottomRight.y;
        const double visibleHeight = std::abs(visibleTop - visibleBottom);
        const double boundsTop = layerBounds.topLeft.y;
        const int startTileTop = std::floor(std::max(topToBottom ? (visibleTop - boundsTop) : (boundsTop - visibleTop), 0.0) / tileWidth);
        const int maxTileTop = std::floor(
                std::max(topToBottom ? (visibleBottom - boundsTop) : (boundsTop - visibleBottom), 0.0) / tileWidth);

        const double maxDisCenterX = visibleWidth * 0.5 + tileWidth;
        const double maxDisCenterY = visibleHeight * 0.5 + tileWidth;
        const double maxDisCenter = std::sqrt(maxDisCenterX * maxDisCenterX + maxDisCenterY * maxDisCenterY);

        for (int x = startTileLeft; x <= maxTileLeft && x < zoomLevelInfo.numTilesX; x++) {
            for (int y = startTileTop; y <= maxTileTop && y < zoomLevelInfo.numTilesY; y++) {
                for (int t = 0; t < zoomLevelInfo.numTilesT; t++) {

                    const Coord minCorner = Coord(layerSystemId, x * tileWidthAdj + boundsLeft, y * tileHeightAdj + boundsTop, 0);
                    const Coord maxCorner = Coord(layerSystemId, minCorner.x + tileWidthAdj, minCorner.y + tileHeightAdj, 0);
                    const RectCoord rect(Coord(layerSystemId,
                                         leftToRight ? minCorner.x : maxCorner.x,
                                         topToBottom ? minCorner.y : maxCorner.y,
                                         0.0),
                                   Coord(layerSystemId,
                                         leftToRight ? maxCorner.x : minCorner.x,
                                         topToBottom ? maxCorner.y : minCorner.y,
                                         0.0));

                    const double tileCenterX = minCorner.x + 0.5f * (maxCorner.x - minCorner.x);
                    const double tileCenterY = minCorner.y + 0.5f * (maxCorner.y - minCorner.y);
                    const double tileCenterDis = std::sqrt(std::pow(tileCenterX - centerVisibleX, 2.0) + std::pow(tileCenterY - centerVisibleY, 2.0));

                    const int tDis = 1 + std::abs(t - curT);
                    const int priority = std::ceil((tileCenterDis / maxDisCenter) * zPriorityRange)  + tDis * tPriorityRange + zoomInd * zoomPriorityRange;

                    curVisibleTilesVec.push_back(PrioritizedTiled2dMapTileInfo(
                            Tiled2dMapTileInfo(rect, x, y, t, zoomLevelInfo.zoomLevelIdentifier, zoomLevelInfo.zoom),
                            priority));

                    visibleTilesVec.push_back(PrioritizedTiled2dMapTileInfo(
                            Tiled2dMapTileInfo(rect, x, y, t, zoomLevelInfo.zoomLevelIdentifier, zoomLevelInfo.zoom),
                            priority));
                }
            }
        }

        curVisibleTiles.visibleTiles.insert(curVisibleTilesVec.begin(), curVisibleTilesVec.end());

        zoomInd++;


        std::unordered_set<PrioritizedTiled2dMapTileInfo> visibleTiles(visibleTilesVec.begin(), visibleTilesVec.end());
        layers.push_back(curVisibleTiles);
    }

    {
        std::lock_guard<std::recursive_mutex> lock(currentZoomLevelMutex);
        currentZoomLevelIdentifier = targetZoomLevelIdentifier;
        currentTime = curT;
    }

    onVisibleTilesChanged(layers);
    currentViewBounds = visibleBoundsLayer;
}

template<class T, class L, class R>
void Tiled2dMapSource<T, L, R>::onVisibleTilesChanged(const std::vector<VisibleTilesLayer> &pyramid) {
    std::unordered_map<size_t, size_t> tasksToAdd;
    {
        std::unordered_set<Tiled2dMapTileInfo> newCurrentVisibleTiles;

        std::unordered_set<PrioritizedTiled2dMapTileInfo> toAdd;

        int currentZoomLevelIdentifier = 0;
        int curT = 0;
        {
            std::lock_guard<std::recursive_mutex> lock(currentZoomLevelMutex);
            currentZoomLevelIdentifier = this->currentZoomLevelIdentifier;
            curT = this->currentTime;
        }

        int smallestCoveringLevel = 0;
        for (const auto &layer: pyramid) {
            if (layer.visibleTiles.size() > 0) {
                break;
            }
            smallestCoveringLevel++;
        }

        // make sure all tiles on the current zoom level are scheduled to load
        for (const auto &layer: pyramid) {
            if (layer.targetZoomLevelOffset <= 0) {
                for (auto const &tileInfo: layer.visibleTiles) {
                    if (!tileLoadingDecision(tileInfo.tileInfo.zoomIdentifier, currentZoomLevelIdentifier, tileInfo.tileInfo.t, curT, layer.zoomLevel, layer.targetZoomLevelOffset, smallestCoveringLevel)) {
                        continue;
                    }
                    newCurrentVisibleTiles.insert(tileInfo.tileInfo);

                    size_t currentTilesCount = 0;
                    {
                        std::lock_guard<std::recursive_mutex> lock(currentTilesMutex);
                        currentTilesCount = currentTiles.count(tileInfo.tileInfo);
                    }
                    size_t currentlyLoadingCount = 0;
                    {
                        std::lock_guard<std::recursive_mutex> lock(currentlyLoadingMutex);
                        currentlyLoadingCount = currentlyLoading.count(tileInfo.tileInfo);
                    }

                    if (currentTilesCount == 0 && currentlyLoadingCount == 0) {
                        toAdd.insert(tileInfo);
                    }

                }
                continue;
            }
        }


        {
            std::lock_guard<std::recursive_mutex> lock(currentPyramidMutex);
            currentPyramid = pyramid;
        }

        {
            std::lock_guard<std::recursive_mutex> lock(currentVisibleTilesMutex);
            currentVisibleTiles = newCurrentVisibleTiles;
        }

        // we only remove tiles that are not visible anymore directly
        // tile from upper zoom levels will be removed as soon as the correct tiles are loaded
        std::unordered_set<Tiled2dMapTileInfo> toRemove;
        {

            std::unordered_map<Tiled2dMapTileInfo, TileWrapper<R>> currentTilesLocal;
            {
                std::lock_guard<std::recursive_mutex> lock(currentTilesMutex);
				for (const auto &tileEntry : currentTiles) {
					currentTilesLocal.insert(tileEntry);
				}
            }


            const Tiled2dMapZoomLevelInfo &firstZoomLevelInfo = zoomLevelInfos.at(0);

            for (const auto &[tileInfo, tileWrapper] : currentTilesLocal) {
                bool found = false;

                if (tileInfo.zoomIdentifier <= currentZoomLevelIdentifier) {
                    for (const auto &layer: pyramid) {
                        for (auto const &tile: layer.visibleTiles) {
                            if (tileInfo == tile.tileInfo) {
                                found = true;
                                break;
                            }
                        }
                    }
                }

                if (!found) {
                    toRemove.insert(tileInfo);
                }
            }
        }


        {
            for (const auto &removedTile : toRemove) {
                {
                    std::lock_guard<std::recursive_mutex> lock(currentTilesMutex);
                    gpc_free_polygon(&currentTiles.at(removedTile).tilePolygon);
                    currentTiles.erase(removedTile);
                }
                {
                    std::lock_guard<std::recursive_mutex> lock(tilesReadyMutex);
                    readyTiles.erase(removedTile);
                }
                if (errorManager)
                    errorManager->removeError(
                            layerConfig->getTileUrl(removedTile.x, removedTile.y, removedTile.t, removedTile.zoomIdentifier));
            }
        }

        {
            std::lock_guard<std::recursive_mutex> lock(loadingQueueMutex);
            for(auto &[loaderIndex, loadingQueue]: loadingQueues) {
                std::vector<PrioritizedTiled2dMapTileInfo> toAddBack;

                for (auto it = loadingQueue.begin(); it != loadingQueue.end();) {
                    bool found = false;
                    for (const auto &layer: pyramid) {
                        auto entryIt = layer.visibleTiles.find(*it);
                        if (entryIt != layer.visibleTiles.end()) {
                            found = true;
                            toAddBack.push_back(*entryIt);
                            break;
                        }
                    }

                    if (!found) {
                        it = loadingQueue.erase(it);
                        if (errorManager)
                            errorManager->removeError(
                                                      layerConfig->getTileUrl(it->tileInfo.x, it->tileInfo.y, it->tileInfo.t,
                                                                              it->tileInfo.zoomIdentifier));
                    } else {
                        it = loadingQueue.erase(it);
                    }
                }

                for(auto const &e: toAddBack) {
                    loadingQueue.insert(e);
                }
            }
        }


        {
            std::lock_guard<std::recursive_mutex> lock(errorTilesMutex);
            for (auto &[loaderIndex, errors]: errorTiles) {
                for (auto it = errors.begin(); it != errors.end();) {
                    bool found = false;
                    for (const auto &layer: pyramid) {
                        auto visibleTile = layer.visibleTiles.find({it->first.tileInfo, 0});
                        if (visibleTile == layer.visibleTiles.end()) {
                            found = true;
                            break;
                        }
                    }
                    if (found) {
                        if (errorManager) errorManager->removeError(layerConfig->getTileUrl(it->first.tileInfo.x, it->first.tileInfo.y, it->first.tileInfo.t, it->first.tileInfo.zoomIdentifier));
                        it = errors.erase(it);
                    } else {
                        ++it;
                    }
                }
            }
        }

        for (const auto &addedTile : toAdd) {
            bool existsInLoadingQueue = false;
            {
                std::lock_guard<std::recursive_mutex> lock(loadingQueueMutex);
                existsInLoadingQueue = loadingQueues[0].count(addedTile) != 0;
            }
            if (!existsInLoadingQueue) {
                bool foundInErrors = false;
                {
                    std::lock_guard<std::recursive_mutex> lock(errorTilesMutex);
                    for (auto &[loaderIndex, errors]: errorTiles) {
                        if (errors.count(addedTile) != 0) {
                            foundInErrors = true;
                            break;
                        }
                    }
                }
                if (!foundInErrors) {
                    std::lock_guard<std::recursive_mutex> lock(loadingQueueMutex);
                    loadingQueues[0].insert(addedTile);
                }
            }
        }

        std::lock_guard<std::recursive_mutex> lock(loadingQueueMutex);
        for(auto const &[index, loadingQueue]: loadingQueues) {
            size_t loadingQueueTasks = loadingQueue.size();

            size_t errorTilesCount = 0;
            {
                std::lock_guard<std::recursive_mutex> lock(errorTilesMutex);
                errorTilesCount = errorTiles[index].size();
            }
            size_t totalOutstandingTasks = std::min<size_t>(max_parallel_loading_tasks, errorTilesCount + loadingQueueTasks);
            size_t dispatchedTaskCount = 0;
            {
                std::lock_guard<std::recursive_mutex> lock(dispatchedTasksMutex);
                dispatchedTaskCount = dispatchedTasks[index];
            }
            size_t tasks = totalOutstandingTasks > dispatchedTaskCount ? totalOutstandingTasks - dispatchedTaskCount : 0;
            tasksToAdd[index] = tasks;
        }
    }

    for (auto const &[loaderIndex, tasks]: tasksToAdd) {
        for (int taskCount = 0; taskCount < tasks; taskCount++) {
            auto taskIdentifier = "Tiled2dMapSource_loadingTask" + std::to_string(loaderIndex) + "_" + std::to_string(taskCount);
            size_t index = loaderIndex;

            std::weak_ptr<Tiled2dMapSource> weakSelfPtr = std::dynamic_pointer_cast<Tiled2dMapSource>(shared_from_this());
            scheduler->addTask(std::make_shared<LambdaTask>(
                                                            TaskConfig(taskIdentifier, 0, TaskPriority::NORMAL, ExecutionEnvironment::IO), [weakSelfPtr, index] {
                                                                auto selfPtr = weakSelfPtr.lock();
                                                                if (selfPtr) selfPtr->performLoadingTask(index);
                                                            }));
            {
                std::lock_guard<std::recursive_mutex> lock(dispatchedTasksMutex);
                dispatchedTasks[index]++;
            }
        }
    }

    //if we removed tiles, we potentially need to update the tilemasks - also if no new tile is loaded
    updateTileMasks();

    auto listenerPtr = listener.lock();
    if (listenerPtr) {
        listenerPtr->onTilesUpdated();
    }
}

template<class T, class L, class R>
std::optional<PrioritizedTiled2dMapTileInfo> Tiled2dMapSource<T, L, R>::dequeueLoadingTask(size_t loaderIndex) {

    std::optional<PrioritizedTiled2dMapTileInfo> result;
    {
        std::lock_guard<std::recursive_mutex> lock(loadingQueueMutex);

        if (!loadingQueues[loaderIndex].empty()) {
            auto tile = loadingQueues[loaderIndex].begin();
            result = *tile;
            loadingQueues[loaderIndex].erase(tile);
        }
    }

    if (!result.has_value()) {
        std::vector<PrioritizedTiled2dMapTileInfo> readyErrorTiles;
        {
            std::lock_guard<std::recursive_mutex> lock(errorTilesMutex);
            for (auto const &errorTile : errorTiles[loaderIndex]) {
                if (errorTile.second.lastLoad + errorTile.second.delay <= DateHelper::currentTimeMillis()) {
                    readyErrorTiles.push_back(errorTile.first);
                }
            }
        }

        {
            std::lock_guard<std::recursive_mutex> lock(currentlyLoadingMutex);
            for (auto const &errorTile : readyErrorTiles) {
                if (currentlyLoading.count(errorTile.tileInfo) == 0) {
                    currentlyLoading.insert(errorTile.tileInfo);
                    return errorTile;
                }
            }
        }
        return std::nullopt;
    }

    {
        std::lock_guard<std::recursive_mutex> lock(currentlyLoadingMutex);
        currentlyLoading.insert(result->tileInfo);
    }

    return result;
}

template<class T, class L, class R>
void Tiled2dMapSource<T, L, R>::performLoadingTask(size_t loaderIndex) {
    while (auto tile = dequeueLoadingTask(loaderIndex)) {
        {
            std::lock_guard<std::recursive_mutex> lock(tilesReadyMutex);
            readyTiles.erase(tile->tileInfo);
        }

        auto loaderResult = loadTile(tile->tileInfo, loaderIndex);
        auto errorManager = this->errorManager;

        LoaderStatus status = loaderResult.status;

        switch (status) {
            case LoaderStatus::OK: {
                if (errorManager) {
                    errorManager->removeError(layerConfig->getTileUrl(tile->tileInfo.x, tile->tileInfo.y, tile->tileInfo.t, tile->tileInfo.zoomIdentifier));
                }
                bool isVisible;
                {
                    std::lock_guard<std::recursive_mutex> lock(currentVisibleTilesMutex);
                    isVisible = currentVisibleTiles.count(tile->tileInfo);
                }
                if (isVisible) {
                    R da = postLoadingTask(loaderResult, tile->tileInfo);

                    PolygonCoord mask({
                                              tile->tileInfo.bounds.topLeft,
                                              Coord(tile->tileInfo.bounds.topLeft.systemIdentifier, tile->tileInfo.bounds.bottomRight.x,
                                                    tile->tileInfo.bounds.topLeft.y, 0),
                                              tile->tileInfo.bounds.bottomRight,
                                              Coord(tile->tileInfo.bounds.topLeft.systemIdentifier, tile->tileInfo.bounds.topLeft.x,
                                                    tile->tileInfo.bounds.bottomRight.y, 0),
                                              tile->tileInfo.bounds.topLeft
                                      }, {});


                    gpc_polygon tilePolygon;
                    gpc_set_polygon({mask}, &tilePolygon);

                    {
                        std::lock_guard<std::recursive_mutex> lock(currentTilesMutex);
                        currentTiles.insert({tile->tileInfo, TileWrapper<R>(da, std::vector<::PolygonCoord>{  }, mask, tilePolygon)});
                    }

                    {
                        std::lock_guard<std::recursive_mutex> lock(errorTilesMutex);
                        errorTiles[loaderIndex].erase(*tile);
                    }
                }
                break;
            }
            case LoaderStatus::NOOP: {
                std::unordered_map<size_t, size_t> tasksToAdd;
                {
                    std::lock_guard<std::recursive_mutex> lock(loadingQueueMutex);
                    auto newLoaderIndex = loaderIndex + 1;
                    if (newLoaderIndex < loadingQueues.size()) {
                        loadingQueues[newLoaderIndex].insert(*tile);


                        for(auto const &[index, loadingQueue]: loadingQueues) {
                            size_t loadingQueueTasks = loadingQueue.size();

                            size_t errorTilesCount = 0;
                            {
                                std::lock_guard<std::recursive_mutex> lock(errorTilesMutex);
                                errorTilesCount = errorTiles[index].size();
                            }
                            size_t totalOutstandingTasks = std::min<size_t>(max_parallel_loading_tasks, errorTilesCount + loadingQueueTasks);
                            size_t dispatchedTaskCount = 0;
                            {
                                std::lock_guard<std::recursive_mutex> lock(dispatchedTasksMutex);
                                dispatchedTaskCount = dispatchedTasks[index];
                            }

                            size_t tasks = totalOutstandingTasks > dispatchedTaskCount ? totalOutstandingTasks - dispatchedTaskCount : 0;
                            tasksToAdd[index] = tasks;
                        }
                    }
                }

                for (auto const &[loaderIndex, tasks]: tasksToAdd) {
                    for (int taskCount = 0; taskCount < tasks; taskCount++) {
                        auto taskIdentifier = "Tiled2dMapSource_loadingTask" + std::to_string(loaderIndex) + "_" + std::to_string(taskCount);
                        size_t index = loaderIndex;

                        std::weak_ptr<Tiled2dMapSource> weakSelfPtr = std::dynamic_pointer_cast<Tiled2dMapSource>(shared_from_this());
                        scheduler->addTask(std::make_shared<LambdaTask>(
                                                                        TaskConfig(taskIdentifier, 0, TaskPriority::NORMAL, ExecutionEnvironment::IO), [weakSelfPtr, index] {
                                                                            auto selfPtr = weakSelfPtr.lock();
                                                                            if (selfPtr) selfPtr->performLoadingTask(index);
                                                                        }));
                        {
                            std::lock_guard<std::recursive_mutex> lock(dispatchedTasksMutex);
                            dispatchedTasks[index]++;
                        }
                    }
                }
                break;
            }
            case LoaderStatus::ERROR_400:
            case LoaderStatus::ERROR_404: {
                {
                    std::lock_guard<std::recursive_mutex> lock(notFoundTilesMutex);
                    notFoundTiles.insert(tile->tileInfo);
                }
                if (errorManager) {
                    errorManager->addTiledLayerError(TiledLayerError(status,
                                                                     loaderResult.errorCode,
                                                                     layerConfig->getLayerName(),
                                                                     layerConfig->getTileUrl(tile->tileInfo.x, tile->tileInfo.y, tile->tileInfo.t,tile->tileInfo.zoomIdentifier),
                                                                     false,
                                                                     tile->tileInfo.bounds));
                }
                break;
            }

            case LoaderStatus::ERROR_TIMEOUT:
            case LoaderStatus::ERROR_OTHER:
            case LoaderStatus::ERROR_NETWORK: {
                int64_t delay = 0;
                {
                    std::lock_guard<std::recursive_mutex> lock(errorTilesMutex);
                    if (errorTiles[loaderIndex].count(*tile) != 0) {
                        errorTiles[loaderIndex][*tile].lastLoad = DateHelper::currentTimeMillis();
                        errorTiles[loaderIndex][*tile].delay = std::min(2 * errorTiles[loaderIndex][*tile].delay, MAX_WAIT_TIME);
                    } else {
                        errorTiles[loaderIndex][*tile] = {DateHelper::currentTimeMillis(), MIN_WAIT_TIME};
                    }
                    delay = errorTiles[loaderIndex][*tile].delay;
                }
                {
                    std::lock_guard<std::recursive_mutex> lock(dispatchedTasksMutex);
                    dispatchedTasks[loaderIndex]++;
                }
                if (errorManager) {
                    errorManager->addTiledLayerError(TiledLayerError(status,
                                                                     loaderResult.errorCode,
                                                                     layerConfig->getLayerName(),
                                                                     layerConfig->getTileUrl(tile->tileInfo.x, tile->tileInfo.y, tile->tileInfo.t, tile->tileInfo.zoomIdentifier),
                                                                     true,
                                                                     tile->tileInfo.bounds));
                }

                auto taskIdentifier = "Tiled2dMapSource_loadingErrorTask";

                std::weak_ptr<Tiled2dMapSource> weakSelfPtr = std::dynamic_pointer_cast<Tiled2dMapSource>(shared_from_this());
                scheduler->addTask(std::make_shared<LambdaTask>(
                                                                TaskConfig(taskIdentifier, delay, TaskPriority::NORMAL, ExecutionEnvironment::IO), [weakSelfPtr, loaderIndex] {
                                                                    auto selfPtr = weakSelfPtr.lock();
                                                                    if (selfPtr) selfPtr->performLoadingTask(loaderIndex);
                                                                }));
                break;
            }
        }

        {
            std::lock_guard<std::recursive_mutex> lock(currentlyLoadingMutex);
            currentlyLoading.erase(tile->tileInfo);
        }

        updateTileMasks();

        auto listenerPtr = listener.lock();
        if (listenerPtr) {
            listenerPtr->onTilesUpdated();
        }
    }

    {
        std::lock_guard<std::recursive_mutex> lock(dispatchedTasksMutex);
        dispatchedTasks[loaderIndex]--;
    }
}

template<class T, class L, class R>
TileLoadingDecision Tiled2dMapSource<T, L, R>::tileLoadingDecision(int tileZ, int curZ, int tileT, int curT, int absoluteLevel, int relativeLevel, int smallestCoveringLevel) {
    if (tileZ == curZ && abs(tileT - curT) < 3) {
        return TileLoadingDecision::loadNeeded;
    }
    else if (tileZ == smallestCoveringLevel && abs(tileT - curT) < 20) {
        return TileLoadingDecision::preload;
    }
    else {
        return TileLoadingDecision::ignore;
    }
}

template<class T, class L, class R>
void Tiled2dMapSource<T, L, R>::updateTileMasks() {
    int curT = 0;
    {
        std::lock_guard<std::recursive_mutex> lock(currentZoomLevelMutex);
        curT = this->currentTime;
    }
    updateTileMasks(curT);
    updateTileMasks(curT+1);
}

template<class T, class L, class R>
void Tiled2dMapSource<T, L, R>::updateTileMasks(int localT) {

    if (!zoomInfo.maskTile) {
        return;
    }

    std::lock_guard<std::recursive_mutex> lock(currentTilesMutex);

    if (currentTiles.empty()) {
        return;
    }

    std::vector<Tiled2dMapTileInfo> tilesToRemove;

    int currentZoomLevelIdentifier = 0;
    {
        std::lock_guard<std::recursive_mutex> lock(currentZoomLevelMutex);
        currentZoomLevelIdentifier = this->currentZoomLevelIdentifier;
    }


    gpc_polygon currentTileMask;
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


    for (auto it = currentTiles.rbegin(); it != currentTiles.rend(); it++ ){
        auto &[tileInfo, tileWrapper] = *it;

        if (tileInfo.t != localT) {
//            tileWrapper.isVisible = false;
            continue;
        }

        tileWrapper.isVisible = true;
        {
            std::lock_guard<std::recursive_mutex> lock(tilesReadyMutex);
            if (readyTiles.count(tileInfo) == 0) {
                continue;
            }
        }


        if (tileInfo.zoomIdentifier != currentZoomLevelIdentifier) {

            gpc_polygon polygonDiff;
            bool freePolygonDiff = false;
            if (currentTileMask.num_contours != 0) {
                freePolygonDiff = true;
                gpc_polygon_clip(GPC_DIFF, &tileWrapper.tilePolygon, &currentTileMask, &polygonDiff);
            } else {
                polygonDiff = tileWrapper.tilePolygon;
            }

            if (polygonDiff.contour == NULL) {
                tileWrapper.isVisible = false;
            } else if (zoomInfo.maskTile) {
                gpc_polygon resultingMask;

                gpc_polygon_clip(GPC_INT, &polygonDiff, &currentViewBoundsPolygon, &resultingMask);

                if (resultingMask.contour == NULL) {
                    tileWrapper.isVisible = false;
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
        if (tileWrapper.isVisible) {
            if (isFirst) {
                gpc_set_polygon({ tileWrapper.tileBounds }, &currentTileMask);
                isFirst = false;
            } else {
                gpc_polygon result;
                gpc_polygon_clip(GPC_UNION, &currentTileMask, &tileWrapper.tilePolygon, &result);
                gpc_free_polygon(&currentTileMask);
                currentTileMask = result;
            }
        }
    }
}

template<class T, class L, class R>
void Tiled2dMapSource<T, L, R>::setTileReady(const Tiled2dMapTileInfo &tile) {
    bool needsUpdate = false;
    {
        std::scoped_lock<std::recursive_mutex, std::recursive_mutex> lock(currentTilesMutex, tilesReadyMutex);
        if (readyTiles.count(tile) == 0) {
            if (currentTiles.count(tile) != 0){
                readyTiles.insert(tile);
                needsUpdate = true;
            }
        }
    }
    if (!needsUpdate) { return; }

    auto taskIdentifier = "Tiled2dMapSource_setTileReady";

    std::weak_ptr<Tiled2dMapSource> weakSelfPtr = std::dynamic_pointer_cast<Tiled2dMapSource>(shared_from_this());
    scheduler->addTask(std::make_shared<LambdaTask>(
            TaskConfig(taskIdentifier, 0, TaskPriority::NORMAL, ExecutionEnvironment::COMPUTATION), [weakSelfPtr] {
                auto selfPtr = weakSelfPtr.lock();
                if (!selfPtr) return;
                selfPtr->updateTileMasks();
                auto listenerPtr = selfPtr->listener.lock();
                if (listenerPtr) {
                    listenerPtr->onTilesUpdated();
                }
            }));
}

template<class T, class L, class R>
void Tiled2dMapSource<T, L, R>::setTilesReady(const std::vector<const Tiled2dMapTileInfo> &tiles) {
    bool needsUpdate = false;
    {
        std::scoped_lock<std::recursive_mutex, std::recursive_mutex> lock(currentTilesMutex, tilesReadyMutex);
        for (auto const tile: tiles) {
            if (readyTiles.count(tile) == 0) {
                if (currentTiles.count(tile) != 0){
                    readyTiles.insert(tile);
                    needsUpdate = true;
                }
            }
        }
    }
    if (!needsUpdate) { return; }

    auto taskIdentifier = "Tiled2dMapSource_setTilesReady";

    std::weak_ptr<Tiled2dMapSource> weakSelfPtr = std::dynamic_pointer_cast<Tiled2dMapSource>(shared_from_this());
    scheduler->addTask(std::make_shared<LambdaTask>(
            TaskConfig(taskIdentifier, 0, TaskPriority::NORMAL, ExecutionEnvironment::COMPUTATION), [weakSelfPtr] {
                auto selfPtr = weakSelfPtr.lock();
                if (!selfPtr) return;
                selfPtr->updateTileMasks();
                auto listenerPtr = selfPtr->listener.lock();
                if (listenerPtr) {
                    listenerPtr->onTilesUpdated();
                }
            }));
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
    {
        std::lock_guard<std::recursive_mutex> lock(notFoundTilesMutex);
        if(notFoundTiles.size() > 0) {
            return LayerReadyState::ERROR;
        }
    }

    {
        std::lock_guard<std::recursive_mutex> lock(errorTilesMutex);
        for (auto const &[index, errors]: errorTiles) {
            if (errors.size() > 0) {
                return LayerReadyState::ERROR;
            }
        }
    }

    {
        std::lock_guard<std::recursive_mutex> lock(currentlyLoadingMutex);
        if(pendingUpdates > 0 || !currentlyLoading.empty()) {
            return LayerReadyState::NOT_READY;
        }
    }

    {
        std::lock_guard<std::recursive_mutex> lock(dispatchedTasksMutex);
        for (auto const &[index, tasks]: dispatchedTasks) {
            if (tasks > 0) {
                return LayerReadyState::NOT_READY;
            }
        }
    }

    {
        std::lock_guard<std::recursive_mutex> lock(currentVisibleTilesMutex);
        for(auto& visible : currentVisibleTiles) {
            if(currentTiles.count(visible) == 0) {
                return LayerReadyState::NOT_READY;
            }
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
    std::lock_guard<std::recursive_mutex> lock(errorTilesMutex);

    //set delay to 0 for all error tiles
    for (auto &[loaderIndex, errors]: errorTiles) {
        for(auto &[tile, errorInfo]: errors) {
            errorInfo.delay = 1;
            auto taskIdentifier = "Tiled2dMapSource_loadingErrorTask";
            auto index = loaderIndex;

            {
                std::lock_guard<std::recursive_mutex> lock(dispatchedTasksMutex);
                dispatchedTasks[loaderIndex]++;
            }
            std::weak_ptr<Tiled2dMapSource> weakSelfPtr = std::dynamic_pointer_cast<Tiled2dMapSource>(shared_from_this());
            scheduler->addTask(std::make_shared<LambdaTask>(
                                                            TaskConfig(taskIdentifier, 1, TaskPriority::NORMAL, ExecutionEnvironment::IO), [weakSelfPtr, index] {
                                                                auto selfPtr = weakSelfPtr.lock();
                                                                if (selfPtr) selfPtr->performLoadingTask(index);
                                                            }));
        }
    }

}

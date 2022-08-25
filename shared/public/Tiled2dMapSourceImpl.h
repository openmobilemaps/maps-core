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
#include "Logger.h"

#include <algorithm>

template <class T, class L, class R>
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

template <class T, class L, class R>
void Tiled2dMapSource<T, L, R>::onVisibleBoundsChanged(const ::RectCoord &visibleBounds, double zoom) {
    if (isPaused) {
        return;
    }
    {
        std::lock_guard<std::recursive_mutex> updateLock(updateMutex);
        updateBounds = visibleBounds;
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
                    std::optional<RectCoord> bounds;
                    std::optional<double> zoom;
                    {
                        std::lock_guard<std::recursive_mutex> updateLock(selfPtr->updateMutex);
                        bounds = selfPtr->updateBounds;
                        zoom = selfPtr->updateZoom;
                    }
                    selfPtr->updateFlag.clear();
                    if (bounds.has_value() && zoom.has_value()) {
                        selfPtr->updateCurrentTileset(*bounds, *zoom);
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

template <class T, class L, class R> void Tiled2dMapSource<T, L, R>::updateCurrentTileset(const RectCoord &visibleBounds, double zoom) {
    std::unordered_set<PrioritizedTiled2dMapTileInfo> visibleTiles;

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
    if (targetZoomLayer < 0) targetZoomLayer = (int)numZoomLevels - 1;
    int startZoomLayer = std::max(targetZoomLayer - zoomInfo.numDrawPreviousLayers, 0);

    int zoomInd = 0;
    int zoomPriorityRange = 20;
    for (int i = startZoomLayer; i <= targetZoomLayer; i++) {
        const Tiled2dMapZoomLevelInfo &zoomLevelInfo = zoomLevelInfos.at(i);

        if(minZoomLevelIdentifier.has_value() && zoomLevelInfo.zoomLevelIdentifier < minZoomLevelIdentifier) {
            continue;
        }
        if (maxZoomLevelIdentifier.has_value() && zoomLevelInfo.zoomLevelIdentifier > maxZoomLevelIdentifier) {
            continue;
        }

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
        int maxTileLeft = std::floor(std::max(leftToRight ? (visibleRight - boundsLeft) : (boundsLeft - visibleRight), 0.0) / tileWidth);
        double visibleTop = visibleBoundsLayer.topLeft.y;
        double visibleBottom = visibleBoundsLayer.bottomRight.y;
        double visibleHeight = std::abs(visibleTop - visibleBottom);
        double boundsTop = layerBounds.topLeft.y;
        int startTileTop = std::floor(std::max(topToBottom ? (visibleTop - boundsTop) : (boundsTop - visibleTop), 0.0) / tileWidth);
        int maxTileTop = std::floor(std::max(topToBottom ? (visibleBottom - boundsTop) : (boundsTop - visibleBottom), 0.0) / tileWidth);

        double maxDisCenterX = visibleWidth * 0.5 + tileWidth;
        double maxDisCenterY = visibleHeight * 0.5 + tileWidth;
        double maxDisCenter = std::sqrt(maxDisCenterX * maxDisCenterX + maxDisCenterY * maxDisCenterY);

        for (int x = startTileLeft; x <= maxTileLeft && x < zoomLevelInfo.numTilesX; x++) {
            for (int y = startTileTop; y <= maxTileTop && y < zoomLevelInfo.numTilesY; y++) {
                Coord minCorner = Coord(layerSystemId, x * tileWidthAdj + boundsLeft, y * tileHeightAdj + boundsTop, 0);
                Coord maxCorner = Coord(layerSystemId, minCorner.x + tileWidthAdj, minCorner.y + tileHeightAdj, 0);
                RectCoord rect(Coord(layerSystemId,
                                     leftToRight ? minCorner.x : maxCorner.x,
                                     topToBottom ? minCorner.y : maxCorner.y,
                                     0.0),
                               Coord(layerSystemId,
                                     leftToRight ? maxCorner.x : minCorner.x,
                                     topToBottom ? maxCorner.y : minCorner.y,
                                     0.0));

                double tileCenterX = minCorner.x + 0.5f * (maxCorner.x - minCorner.x);
                double tileCenterY = minCorner.y + 0.5f * (maxCorner.y - minCorner.y);
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
    currentViewBounds = visibleBoundsLayer;
}

template <class T, class L, class R>
void Tiled2dMapSource<T, L, R>::onVisibleTilesChanged(const std::unordered_set<PrioritizedTiled2dMapTileInfo> &visibleTiles) {
    std::unordered_map<size_t, size_t> tasksToAdd;
    {

        std::unordered_set<Tiled2dMapTileInfo> newCurrentVisibleTiles;

        std::unordered_set<PrioritizedTiled2dMapTileInfo> toAdd;
        for (const auto &tileInfo : visibleTiles) {
            newCurrentVisibleTiles.insert(tileInfo.tileInfo);

            int currentTilesCount = 0;
            {
                std::lock_guard<std::recursive_mutex> lock(currentTilesMutex);
                currentTilesCount = currentTiles.count(tileInfo.tileInfo);
            }
            int currentlyLoadingCount = 0;
            {
                std::lock_guard<std::recursive_mutex> lock(currentlyLoadingMutex);
                currentlyLoadingCount = currentlyLoading.count(tileInfo.tileInfo);
            }

            if (currentTilesCount == 0 && currentlyLoadingCount == 0) {
                std::lock_guard<std::recursive_mutex> lock(loadingQueueMutex);
                for(auto const &[loaderIndex, loadingQueue]: loadingQueues) {
                    for (auto it = loadingQueues[loaderIndex].begin(); it != loadingQueues[loaderIndex].end(); it++) {
                        if (it->tileInfo == tileInfo.tileInfo) {
                            loadingQueues[loaderIndex].erase(it);
                            break;
                        }
                    }
                }
                toAdd.insert(tileInfo);
            }
        }

        {
            std::lock_guard<std::recursive_mutex> lock(currentVisibleTilesMutex);
            currentVisibleTiles = newCurrentVisibleTiles;
        }

        std::unordered_set<Tiled2dMapTileInfo> toRemove;
        {
            std::lock_guard<std::recursive_mutex> lock(currentTilesMutex);
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
        }


        for (const auto &removedTile : toRemove) {
            {
                std::lock_guard<std::recursive_mutex> lock(currentTilesMutex);
                currentTiles.erase(removedTile);
            }
            if (errorManager) errorManager->removeError(layerConfig->getTileUrl(removedTile.x, removedTile.y, removedTile.zoomIdentifier));
        }

        {
            std::lock_guard<std::recursive_mutex> lock(loadingQueueMutex);
            for(auto &[loaderIndex, loadingQueue]: loadingQueues) {
                for (auto it = loadingQueue.begin(); it != loadingQueue.end();) {
                    if (visibleTiles.count(*it) == 0) {
                        if (errorManager) errorManager->removeError(layerConfig->getTileUrl(it->tileInfo.x, it->tileInfo.y, it->tileInfo.zoomIdentifier));
                        it = loadingQueue.erase(it);
                    } else {
                        ++it;
                    }
                }
            }
        }

        {
            std::lock_guard<std::recursive_mutex> lock(errorTilesMutex);
            for (auto &[loaderIndex, errors]: errorTiles) {
                for (auto it = errors.begin(); it != errors.end();) {
                    auto visibleTile = visibleTiles.find({it->first.tileInfo, 0});
                    if (visibleTile == visibleTiles.end()) {
                        if (errorManager) errorManager->removeError(layerConfig->getTileUrl(it->first.tileInfo.x, it->first.tileInfo.y, it->first.tileInfo.zoomIdentifier));
                        it = errors.erase(it);
                    } else {
                        ++it;
                    }
                }
            }
        }

        {
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


    auto listenerPtr = listener.lock();
    if (listenerPtr) {
        listenerPtr->onTilesUpdated();
    }
}

template <class T, class L, class R> std::optional<PrioritizedTiled2dMapTileInfo> Tiled2dMapSource<T, L, R>::dequeueLoadingTask(size_t loaderIndex) {
    std::lock_guard<std::recursive_mutex> lock(loadingQueueMutex);

    if (loadingQueues[loaderIndex].empty()) {
        std::lock_guard<std::recursive_mutex> lock(errorTilesMutex);
        for (auto const &errorTile : errorTiles[loaderIndex]) {
            std::lock_guard<std::recursive_mutex> lock(currentlyLoadingMutex);
            if (errorTile.second.lastLoad + errorTile.second.delay <= DateHelper::currentTimeMillis() &&
                currentlyLoading.count(errorTile.first.tileInfo) == 0) {
                currentlyLoading.insert(errorTile.first.tileInfo);
                return errorTile.first;
            }
        }
        return std::nullopt;
    }

    auto tile = *loadingQueues[loaderIndex].begin();
    auto tileInfo = tile.tileInfo;
    loadingQueues[loaderIndex].erase(tile);

    {
        std::lock_guard<std::recursive_mutex> lock(currentlyLoadingMutex);
        currentlyLoading.insert(tileInfo);
    }

    return tile;
}

template <class T, class L, class R> void Tiled2dMapSource<T, L, R>::performLoadingTask(size_t loaderIndex) {
    while (auto tile = dequeueLoadingTask(loaderIndex)) {
        auto loaderResult = loadTile(tile->tileInfo, loaderIndex);
        auto errorManager = this->errorManager;

        LoaderStatus status = loaderResult.status;

        switch (status) {
            case LoaderStatus::OK: {
                if (errorManager) {
                    errorManager->removeError(layerConfig->getTileUrl(tile->tileInfo.x, tile->tileInfo.y, tile->tileInfo.zoomIdentifier));
                }
                bool isVisible;
                {
                    std::lock_guard<std::recursive_mutex> lock(currentVisibleTilesMutex);
                    isVisible = currentVisibleTiles.count(tile->tileInfo);
                }
                if (isVisible) {
                    R da = postLoadingTask(loaderResult, tile->tileInfo);

                    {
                        std::lock_guard<std::recursive_mutex> lock(currentTilesMutex);
                        currentTiles[tile->tileInfo] = std::move(da);
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
                                                                     layerConfig->getTileUrl(tile->tileInfo.x, tile->tileInfo.y, tile->tileInfo.zoomIdentifier),
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
                    {
                        std::lock_guard<std::recursive_mutex> lock(errorTilesMutex);
                        dispatchedTasks[loaderIndex]++;
                    }
                }
                if (errorManager) {
                    errorManager->addTiledLayerError(TiledLayerError(status,
                                                                     loaderResult.errorCode,
                                                                     layerConfig->getLayerName(),
                                                                     layerConfig->getTileUrl(tile->tileInfo.x, tile->tileInfo.y, tile->tileInfo.zoomIdentifier),
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

        auto listenerPtr = listener.lock();
        if (listenerPtr) {
            listenerPtr->onTilesUpdated();
        }

        {
            std::lock_guard<std::recursive_mutex> lock(currentlyLoadingMutex);
            currentlyLoading.erase(tile->tileInfo);
        }
    }
    {
        std::lock_guard<std::recursive_mutex> lock(dispatchedTasksMutex);
        dispatchedTasks[loaderIndex]--;
    }
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

        for(auto& visible : currentVisibleTiles) {
            if(currentTiles.count(visible) == 0) {
                return LayerReadyState::NOT_READY;
            }
        }
    }

    return LayerReadyState::READY;
}

template<class T, class L, class R>
void Tiled2dMapSource<T, L, R>::setErrorManager(const std::shared_ptr<::ErrorManager> & errorManager) {
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

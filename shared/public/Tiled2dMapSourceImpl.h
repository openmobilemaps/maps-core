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

template <class T, class L, class R>
Tiled2dMapSource<T, L, R>::Tiled2dMapSource(const MapConfig &mapConfig, const std::shared_ptr<Tiled2dMapLayerConfig> &layerConfig,
                                         const std::shared_ptr<CoordinateConversionHelperInterface> &conversionHelper,
                                         const std::shared_ptr<SchedulerInterface> &scheduler,
                                         const std::shared_ptr<Tiled2dMapSourceListenerInterface> &listener,
                                         float screenDensityPpi)
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
    std::lock_guard<std::recursive_mutex> lock(tilesMutex);
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
    int startZoomLayer = 0;
    int endZoomLevel = std::min((int)numZoomLevels - 1, targetZoomLayer + 2);

    int zoomInd = 0;
    int zoomPriorityRange = 20;

    std::vector<VisibleTilesLayer> layers;

    for (int i = startZoomLayer; i <= endZoomLevel; i++) {
        const Tiled2dMapZoomLevelInfo &zoomLevelInfo = zoomLevelInfos.at(i);

        if(minZoomLevelIdentifier.has_value() && zoomLevelInfo.zoomLevelIdentifier < minZoomLevelIdentifier) {
            continue;
        }
        if (maxZoomLevelIdentifier.has_value() && zoomLevelInfo.zoomLevelIdentifier > maxZoomLevelIdentifier) {
            continue;
        }

        VisibleTilesLayer curVisibleTiles(i - targetZoomLayer);

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

                curVisibleTiles.visibleTiles.insert(PrioritizedTiled2dMapTileInfo(
                                                                               Tiled2dMapTileInfo(rect, x, y, zoomLevelInfo.zoomLevelIdentifier, i),
                                                                               std::ceil((tileCenterDis / maxDisCenter) * zoomPriorityRange) + zoomInd * zoomPriorityRange));

                visibleTiles.insert(PrioritizedTiled2dMapTileInfo(
                    Tiled2dMapTileInfo(rect, x, y, zoomLevelInfo.zoomLevelIdentifier, i),
                    std::ceil((tileCenterDis / maxDisCenter) * zoomPriorityRange) + zoomInd * zoomPriorityRange));
            }
        }

        zoomInd++;

        layers.push_back(curVisibleTiles);
    }

    onVisibleTilesChanged(layers);
    currentViewBounds = visibleBoundsLayer;
}

template <class T, class L, class R>
void Tiled2dMapSource<T, L, R>::onVisibleTilesChanged(const std::vector<VisibleTilesLayer> &pyramid) {
    size_t tasksToAdd;
    {
        std::lock_guard<std::recursive_mutex> lock(tilesMutex);
        currentPyramid = pyramid;

        currentVisibleTiles.clear();

        std::unordered_set<PrioritizedTiled2dMapTileInfo> toAdd;

        // make sure all tiles on the current zoom level are scheduled to load
        for (const auto &layer: pyramid) {
            if (layer.targetZoomLevelOffset == 0) {
                for (auto const &tileInfo: layer.visibleTiles) {
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
                continue;
            }
        }

        // we only remove tiles that are not visible anymore directly
        // tile from upper zoom levels will be removed as soon as the correct tiles are loaded
        std::unordered_set<Tiled2dMapTileInfo> toRemove;
        for (const auto &tileEntry : currentTiles) {
            bool found = false;
            for (const auto &layer: pyramid) {
                for (auto const &tile: layer.visibleTiles) {
                    if (tileEntry.first == tile.tileInfo) {
                        found = true;
                        break;
                    }
                }
            }

            if (!found) {
                toRemove.insert(tileEntry.first);
            }
        }

        {
            std::lock_guard<std::recursive_mutex> lock(tilesReadyMutex);
            for (const auto &removedTile : toRemove) {
                currentTiles.erase(removedTile);
                readyTiles.erase(removedTile);
			    if (errorManager) errorManager->removeError(layerConfig->getTileUrl(removedTile.x, removedTile.y, removedTile.zoomIdentifier));
            }
        }

        for (auto it = loadingQueue.begin(); it != loadingQueue.end();) {
            bool found = false;
            for (const auto &layer: pyramid) {
                found = layer.visibleTiles.count(*it) != 0;
                if (found) {
                    break;
                }
            }

            if (found) {
                it = loadingQueue.erase(it);
				if (errorManager) errorManager->removeError(layerConfig->getTileUrl(it->tileInfo.x, it->tileInfo.y, it->tileInfo.zoomIdentifier));
            } else {
                ++it;
            }
        }

        for (auto it = errorTiles.begin(); it != errorTiles.end();) {
            bool found = false;
            for (const auto &layer: pyramid) {
                found = layer.visibleTiles.count({it->first, 0}) != 0;
                if (found) {
                    break;
                }
            }
            if (found) {
				if (errorManager) errorManager->removeError(layerConfig->getTileUrl(it->first.x, it->first.y, it->first.zoomIdentifier));
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
        tasksToAdd = totalOutstandingTasks > dispatchedTaskCount ? totalOutstandingTasks - dispatchedTaskCount : 0;
    }
    for (int taskCount = 0; taskCount < tasksToAdd; taskCount++) {
        auto taskIdentifier = "Tiled2dMapSource_loadingTask" + std::to_string(taskCount);

        std::weak_ptr<Tiled2dMapSource> weakSelfPtr = std::dynamic_pointer_cast<Tiled2dMapSource>(shared_from_this());
        scheduler->addTask(std::make_shared<LambdaTask>(
            TaskConfig(taskIdentifier, 0, TaskPriority::NORMAL, ExecutionEnvironment::IO), [weakSelfPtr] {
                auto selfPtr = weakSelfPtr.lock();
                if (selfPtr) selfPtr->performLoadingTask();
            }));
        dispatchedTasks++;
    }

    auto listenerPtr = listener.lock();
    if (listenerPtr) {
        listenerPtr->onTilesUpdated();
    }
}

template <class T, class L, class R> std::optional<Tiled2dMapTileInfo> Tiled2dMapSource<T, L, R>::dequeueLoadingTask() {
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

template <class T, class L, class R> void Tiled2dMapSource<T, L, R>::performLoadingTask() {
    if (auto tile = dequeueLoadingTask()) {
        {
            std::lock_guard<std::recursive_mutex> lock(tilesReadyMutex);
            readyTiles.erase(*tile);
        }
        auto loaderResult = loadTile(*tile);
        auto errorManager = this->errorManager;

        LoaderStatus status = loaderResult.status;

        switch (status) {
            case LoaderStatus::OK: {
                if (errorManager) {
                    errorManager->removeError(layerConfig->getTileUrl(tile->x, tile->y, tile->zoomIdentifier));
                }
                bool isVisible;
                {
                    std::lock_guard<std::recursive_mutex> lock(tilesMutex);
                    isVisible = currentVisibleTiles.count(*tile);
                }
                if (isVisible) {
                    R da = postLoadingTask(loaderResult, *tile);

                    std::lock_guard<std::recursive_mutex> lock(tilesMutex);
                    PolygonCoord mask({
                            tile->bounds.topLeft,
                            Coord(tile->bounds.topLeft.systemIdentifier, tile->bounds.bottomRight.x, tile->bounds.topLeft.y, 0),
                            tile->bounds.bottomRight,
                            Coord(tile->bounds.topLeft.systemIdentifier, tile->bounds.topLeft.x, tile->bounds.bottomRight.y, 0),
                            tile->bounds.topLeft
                    }, {});

                    currentTiles.insert({*tile, TileWrapper<R>(da, mask)});

                    errorTiles.erase(*tile);
                }
                break;
            }
            case LoaderStatus::ERROR_400:
            case LoaderStatus::ERROR_404: {
                std::lock_guard<std::recursive_mutex> lock(tilesMutex);
                notFoundTiles.insert(*tile);
                if (errorManager) {
                    errorManager->addTiledLayerError(TiledLayerError(status,
                                                                     loaderResult.errorCode,
                                                                     layerConfig->getLayerName(),
                                                                     layerConfig->getTileUrl(tile->x, tile->y, tile->zoomIdentifier),
                                                                     false,
                                                                     tile->bounds));
                }
                break;
            }

            case LoaderStatus::ERROR_TIMEOUT:
            case LoaderStatus::ERROR_OTHER:
            case LoaderStatus::ERROR_NETWORK: {
                int64_t delay = 0;
                {
                    std::lock_guard<std::recursive_mutex> lock(tilesMutex);
                    if (errorTiles.count(*tile) != 0) {
                        errorTiles[*tile].lastLoad = DateHelper::currentTimeMillis();
                        errorTiles[*tile].delay = std::min(2 * errorTiles[*tile].delay, MAX_WAIT_TIME);
                    } else {
                        errorTiles[*tile] = {DateHelper::currentTimeMillis(), MIN_WAIT_TIME};
                    }
                    delay = errorTiles[*tile].delay;
                    dispatchedTasks++;
                }
                if (errorManager) {
                    errorManager->addTiledLayerError(TiledLayerError(status,
                                                                     loaderResult.errorCode,
                                                                     layerConfig->getLayerName(),
                                                                     layerConfig->getTileUrl(tile->x, tile->y, tile->zoomIdentifier),
                                                                     true,
                                                                     tile->bounds));
                }

                auto taskIdentifier = "Tiled2dMapSource_loadingErrorTask";

                std::weak_ptr<Tiled2dMapSource> weakSelfPtr = std::dynamic_pointer_cast<Tiled2dMapSource>(shared_from_this());
                scheduler->addTask(std::make_shared<LambdaTask>(
                        TaskConfig(taskIdentifier, delay, TaskPriority::NORMAL, ExecutionEnvironment::IO), [weakSelfPtr] {
                            auto selfPtr = weakSelfPtr.lock();
                            if (selfPtr) selfPtr->performLoadingTask();
                        }));
                break;
            }
        }

        {
            std::lock_guard<std::recursive_mutex> lock(tilesMutex);
            currentlyLoading.erase(*tile);

            updateTileMasks();
        }

        auto listenerPtr = listener.lock();
        if (listenerPtr) {
            listenerPtr->onTilesUpdated();
        }

        {
            std::lock_guard<std::recursive_mutex> lock(tilesMutex);
            currentlyLoading.erase(*tile);
        }
    }
}

template<class T, class L, class R>
void Tiled2dMapSource<T, L, R>::updateTileMasks() {

    if (!zoomInfo.maskTile) {
        return;
    }

    std::lock_guard<std::recursive_mutex> lock(tilesMutex);

    // now we can check if we can delete a tile from a upper zoom level
    std::vector<Tiled2dMapTileInfo> tilesFromDifferentZoomLevel;
    std::vector<Tiled2dMapTileInfo> otherLoadedTiles;
    {
        std::lock_guard<std::recursive_mutex> lock(tilesReadyMutex);
        for(auto const &[t, tex] : currentTiles) {
            if (currentVisibleTiles.count(t) == 0) {
                tilesFromDifferentZoomLevel.push_back(t);
            } else {
                if (readyTiles.count(t) != 0) {
                    otherLoadedTiles.push_back(t);
                }
            }
        }

    }

    std::vector<::Coord> viewBounds{
        currentViewBounds.topLeft,
        Coord(currentViewBounds.topLeft.systemIdentifier, currentViewBounds.bottomRight.x, currentViewBounds.topLeft.y, 0),
        currentViewBounds.bottomRight,
        Coord(currentViewBounds.topLeft.systemIdentifier, currentViewBounds.topLeft.x, currentViewBounds.bottomRight.y, 0),
        currentViewBounds.topLeft // TODO: is the first point really needed?
    };

    for (auto const &otht: otherLoadedTiles) {
        std::vector<::Coord> p{
            otht.bounds.topLeft,
            Coord(otht.bounds.topLeft.systemIdentifier, otht.bounds.bottomRight.x, otht.bounds.topLeft.y, 0),
            otht.bounds.bottomRight,
            Coord(otht.bounds.topLeft.systemIdentifier, otht.bounds.topLeft.x, otht.bounds.bottomRight.y, 0),
            otht.bounds.topLeft // TODO: is the first point really needed?
        };
        currentTiles.at(otht).mask = PolygonCoord(p, {});
    }

    for (auto const &tfdz: tilesFromDifferentZoomLevel) {

        std::vector<::Coord> subj{
            tfdz.bounds.topLeft,
            Coord(tfdz.bounds.topLeft.systemIdentifier, tfdz.bounds.bottomRight.x, tfdz.bounds.topLeft.y, 0),
            tfdz.bounds.bottomRight,
            Coord(tfdz.bounds.topLeft.systemIdentifier, tfdz.bounds.topLeft.x, tfdz.bounds.bottomRight.y, 0),
            tfdz.bounds.topLeft // TODO: is the first point really needed?
        };

        gpc_polygon otherTiles;
        otherTiles.num_contours = 0;
        bool isFirst = true;
        for (auto const &olt: otherLoadedTiles) {
            std::vector<std::vector<::Coord>> clip;
            clip.push_back({
                olt.bounds.topLeft,
                Coord(olt.bounds.topLeft.systemIdentifier, olt.bounds.bottomRight.x, olt.bounds.topLeft.y, 0),
                olt.bounds.bottomRight,
                Coord(olt.bounds.topLeft.systemIdentifier, olt.bounds.topLeft.x, olt.bounds.bottomRight.y, 0),
                olt.bounds.topLeft // TODO: is the first point really needed?
            });
            if (isFirst) {
                gpc_set_polygon(clip, &otherTiles);
                isFirst = false;
            } else {
                gpc_polygon additional, result;
                gpc_set_polygon(clip, &additional);
                gpc_polygon_clip(GPC_UNION, &otherTiles, &additional, &result);
                gpc_free_polygon(&otherTiles);
                gpc_free_polygon(&additional);
                otherTiles = result;
            }
        }

        gpc_polygon viewB, subject, result, intermediate;
        gpc_set_polygon({subj}, &subject);
        gpc_set_polygon({viewBounds}, &viewB);
        bool freeIntermediate = false;
        if (otherTiles.num_contours != 0) {
            freeIntermediate = true;
            gpc_polygon_clip(GPC_DIFF, &subject, &otherTiles, &intermediate);
        } else {
            intermediate = subject;
        }
        gpc_polygon_clip(GPC_INT, &viewB, &intermediate, &result);

        if (result.contour == NULL) {
            currentTiles.erase(tfdz);
        } else {
            currentTiles.at(tfdz).mask = gpc_get_polygon_coord(&intermediate, CoordinateSystemIdentifiers::EPSG4326());
        }

        if (freeIntermediate) {
            gpc_free_polygon (&intermediate);
        }
        gpc_free_polygon (&subject);
        gpc_free_polygon (&result);
        gpc_free_polygon (&viewB);
    }
}

template<class T, class L, class R>
void Tiled2dMapSource<T, L, R>::setTileReady(const Tiled2dMapTileInfo &tile) {
    bool needsUpdate = false;
    {
        std::lock_guard<std::recursive_mutex> lock(tilesReadyMutex);
        if (readyTiles.count(tile) == 0) {
            readyTiles.insert(tile);
            needsUpdate = true;
        }
    }
    if (!needsUpdate) { return; }
    updateTileMasks();

    auto listenerPtr = listener.lock();
    if (listenerPtr) {
        listenerPtr->onTilesUpdated();
    }
}

template<class T, class L, class R>
void Tiled2dMapSource<T, L, R>::setTilesReady(const std::vector<const Tiled2dMapTileInfo> &tiles) {
    bool needsUpdate = false;
    {
        std::lock_guard<std::recursive_mutex> lock(tilesReadyMutex);
        for(auto const tile: tiles) {
            if (readyTiles.count(tile) == 0) {
                readyTiles.insert(tile);
                needsUpdate = true;
            }
        }
    }
    if (!needsUpdate) { return; }
    updateTileMasks();

    auto listenerPtr = listener.lock();
    if (listenerPtr) {
        listenerPtr->onTilesUpdated();
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
    std::lock_guard<std::recursive_mutex> lock(tilesMutex);

    if(errorTiles.size() > 0 || notFoundTiles.size() > 0) {
        return LayerReadyState::ERROR;
    }

    if(pendingUpdates > 0 || dispatchedTasks > 0 || !currentlyLoading.empty()) {
        return LayerReadyState::NOT_READY;
    }

    for(auto& visible : currentVisibleTiles) {
        if(currentTiles.count(visible) == 0) {
            return LayerReadyState::NOT_READY;
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
    std::lock_guard<std::recursive_mutex> lock(tilesMutex);

    //set delay to 0 for all error tiles
    for(auto &[tile, errorInfo]: errorTiles) {
        errorInfo.delay = 1;
        auto taskIdentifier = "Tiled2dMapSource_loadingErrorTask";

        dispatchedTasks++;
        std::weak_ptr<Tiled2dMapSource> weakSelfPtr = std::dynamic_pointer_cast<Tiled2dMapSource>(shared_from_this());
        scheduler->addTask(std::make_shared<LambdaTask>(
                                                        TaskConfig(taskIdentifier, 1, TaskPriority::NORMAL, ExecutionEnvironment::IO), [weakSelfPtr] {
                                                            auto selfPtr = weakSelfPtr.lock();
                                                            if (selfPtr) selfPtr->performLoadingTask();
                                                        }));
    }

}

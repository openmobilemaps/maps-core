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
                                         float screenDensityPpi,
                                         size_t loaderCount)
    : mapConfig(mapConfig)
    , layerConfig(layerConfig)
    , conversionHelper(conversionHelper)
    , scheduler(scheduler)
    , zoomLevelInfos(layerConfig->getZoomLevelInfos())
    , zoomInfo(layerConfig->getZoomInfo())
    , layerSystemId(layerConfig->getCoordinateSystemIdentifier())
    , isPaused(false)
    , screenDensityPpi(screenDensityPpi) {
    std::sort(zoomLevelInfos.begin(), zoomLevelInfos.end(),
              [](const Tiled2dMapZoomLevelInfo &a, const Tiled2dMapZoomLevelInfo &b) -> bool { return a.zoom > b.zoom; });
}

template<class T, class L, class R>
bool Tiled2dMapSource<T, L, R>::isTileVisible(const Tiled2dMapTileInfo &tileInfo) {
    return currentVisibleTiles.count(tileInfo) > 0;
}

template<class T, class L, class R>
void Tiled2dMapSource<T, L, R>::onVisibleBoundsChanged(const ::RectCoord &visibleBounds, int curT, double zoom) {
    if (isPaused) {
        return;
    }

    std::vector<PrioritizedTiled2dMapTileInfo> visibleTilesVec;

    RectCoord visibleBoundsLayer = conversionHelper->convertRect(layerSystemId, visibleBounds);

    double centerVisibleX = visibleBoundsLayer.topLeft.x + 0.5 * (visibleBoundsLayer.bottomRight.x - visibleBoundsLayer.topLeft.x);
    double centerVisibleY = visibleBoundsLayer.topLeft.y + 0.5 * (visibleBoundsLayer.bottomRight.y - visibleBoundsLayer.topLeft.y);

    size_t numZoomLevels = zoomLevelInfos.size();
    int targetZoomLayer = -1;

    // Each pixel is assumed to be 0.28mm – https://gis.stackexchange.com/a/315989
    const float screenScaleFactor = zoomInfo.adaptScaleToScreen ? screenDensityPpi / (0.0254 / 0.00028) : 1.0;

    if (!zoomInfo.underzoom
        && (zoomLevelInfos.empty() || zoomLevelInfos[0].zoom * zoomInfo.zoomLevelScaleFactor * screenScaleFactor < zoom)) {
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

    int distanceWeight = 100;
    int zoomLevelWeight = 1000 * zoomLevelInfos.at(0).numTilesT;
    int zDistanceWeight = 1000 * zoomLevelInfos.at(0).numTilesT;

    std::vector<VisibleTilesLayer> layers;

    for (int i = startZoomLayer; i <= endZoomLevel; i++) {
        const Tiled2dMapZoomLevelInfo &zoomLevelInfo = zoomLevelInfos.at(i);

        if (minZoomLevelIdentifier.has_value() && zoomLevelInfo.zoomLevelIdentifier < minZoomLevelIdentifier) {
            continue;
        }
        if (maxZoomLevelIdentifier.has_value() && zoomLevelInfo.zoomLevelIdentifier > maxZoomLevelIdentifier) {
            continue;
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

                    if( t != curT ) {
                        continue;
                    }

                    const Coord topLeft = Coord(layerSystemId, x * tileWidthAdj + boundsLeft, y * tileHeightAdj + boundsTop, 0);
                    const Coord bottomRight = Coord(layerSystemId, topLeft.x + tileWidthAdj, topLeft.y + tileHeightAdj, 0);

                    const double tileCenterX = topLeft.x + 0.5f * tileWidthAdj;
                    const double tileCenterY = topLeft.y + 0.5f * tileHeightAdj;
                    const double tileCenterDis = std::sqrt(std::pow(tileCenterX - centerVisibleX, 2.0) + std::pow(tileCenterY - centerVisibleY, 2.0));

                    const int zDis = 1 + std::abs(t - curT);

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

    onVisibleTilesChanged(layers);
    currentViewBounds = visibleBoundsLayer;
}

template<class T, class L, class R>
void Tiled2dMapSource<T, L, R>::onVisibleTilesChanged(const std::vector<VisibleTilesLayer> &pyramid) {

    currentVisibleTiles.clear();

    std::vector<PrioritizedTiled2dMapTileInfo> toAdd;

    // make sure all tiles on the current zoom level are scheduled to load
    for (const auto &layer: pyramid) {
        if (layer.targetZoomLevelOffset <= 0 && layer.targetZoomLevelOffset >= -zoomInfo.numDrawPreviousLayers){
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

    // we only remove tiles that are not visible anymore directly
    // tile from upper zoom levels will be removed as soon as the correct tiles are loaded if mask tiles is enabled
    std::vector<Tiled2dMapTileInfo> toRemove;

    int currentZoomLevelIdentifier = this->currentZoomLevelIdentifier;
    for (const auto &[tileInfo, tileWrapper] : currentTiles) {
        bool found = false;

        if (tileInfo.zoomIdentifier <= currentZoomLevelIdentifier) {
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

    currentlyLoading.insert({tile, loaderIndex});
    readyTiles.erase(tile);
    
    std::weak_ptr<Tiled2dMapSource> weakSelfPtr = std::dynamic_pointer_cast<Tiled2dMapSource>(shared_from_this());
    auto weakActor = WeakActor<Tiled2dMapSource>(mailbox, std::static_pointer_cast<Tiled2dMapSource>(shared_from_this()));
    
     loadDataAsync(tile, loaderIndex).then([weakActor, loaderIndex, tile](::djinni::Future<L> result) {
        weakActor.message(&Tiled2dMapSource::didLoad, tile, loaderIndex, result.get());
    });
}

template<class T, class L, class R>
void Tiled2dMapSource<T, L, R>::didLoad(Tiled2dMapTileInfo tile, size_t loaderIndex, const L &loaderResult) {
    currentlyLoading.erase(tile);

    const bool isVisible = currentVisibleTiles.count(tile);
    if (!isVisible) {
        errorTiles[loaderIndex].erase(tile);
        return;
    }

    auto errorManager = this->errorManager;

    LoaderStatus status = loaderResult.status;
    
    switch (status) {
        case LoaderStatus::OK: {
            if (errorManager) {
                errorManager->removeError(layerConfig->getTileUrl(tile.x, tile.y, tile.t, tile.zoomIdentifier));
            }

            R da = postLoadingTask(loaderResult, tile);

            auto bounds = tile.bounds;
            PolygonCoord mask({ bounds.topLeft,
                Coord(bounds.topLeft.systemIdentifier, bounds.bottomRight.x, bounds.topLeft.y, 0),
                bounds.bottomRight,
                Coord(bounds.topLeft.systemIdentifier, bounds.topLeft.x, bounds.bottomRight.y, 0),
                bounds.topLeft }, {});

            gpc_polygon tilePolygon;
            gpc_set_polygon({mask}, &tilePolygon);

            currentTiles.insert({tile, TileWrapper<R>(da, std::vector<::PolygonCoord>{  }, mask, tilePolygon)});

            errorTiles[loaderIndex].erase(tile);
            
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
                                                                 loaderResult.errorCode,
                                                                 layerConfig->getLayerName(),
                                                                 layerConfig->getTileUrl(tile.x, tile.y, tile.t,tile.zoomIdentifier),
                                                                 false,
                                                                 tile.bounds));
            }
            break;
        }
            
        case LoaderStatus::ERROR_TIMEOUT:
        case LoaderStatus::ERROR_OTHER:
        case LoaderStatus::ERROR_NETWORK: {
            int64_t delay = 0;
            if (errorTiles[loaderIndex].count(tile) != 0) {
                errorTiles[loaderIndex].at(tile).lastLoad = DateHelper::currentTimeMillis();
                errorTiles[loaderIndex].at(tile).delay = std::min(2 * errorTiles[loaderIndex].at(tile).delay, MAX_WAIT_TIME);
            } else {
                errorTiles[loaderIndex][tile] = {DateHelper::currentTimeMillis(), MIN_WAIT_TIME};
            }
            delay = errorTiles[loaderIndex].at(tile).delay;
           
            
            if (errorManager) {
                errorManager->addTiledLayerError(TiledLayerError(status,
                                                                 loaderResult.errorCode,
                                                                 layerConfig->getLayerName(),
                                                                 layerConfig->getTileUrl(tile.x, tile.y, tile.t, tile.zoomIdentifier),
                                                                 true,
                                                                 tile.bounds));
            }

            auto taskIdentifier = "Tiled2dMapSource_loadingErrorTask";


            auto weakActor = WeakActor<Tiled2dMapSource>(mailbox, std::dynamic_pointer_cast<Tiled2dMapSource>(shared_from_this()));
            scheduler->addTask(std::make_shared<LambdaTask>(TaskConfig(taskIdentifier, delay, TaskPriority::NORMAL, ExecutionEnvironment::IO), [weakActor, tile, loaderIndex] {
                weakActor.message(&Tiled2dMapSource::performLoadingTask, tile, loaderIndex);
            }));

            break;
        }
    }

    updateTileMasks();
    
    notifyTilesUpdates();
}

template<class T, class L, class R>
void Tiled2dMapSource<T, L, R>::updateTileMasks() {

    if (!zoomInfo.maskTile) {
        return;
    }


    if (currentTiles.empty()) {
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

    for (auto it = currentTiles.rbegin(); it != currentTiles.rend(); it++ ){
        auto &[tileInfo, tileWrapper] = *it;

        tileWrapper.isVisible = true;

        if (tileInfo.zoomIdentifier != currentZoomLevelIdentifier) {

            if (currentTileMask.num_contours != 0) {
                if(!completeViewBoundsDrawn) {
                    gpc_polygon diff;
                    gpc_polygon_clip(GPC_DIFF, &currentViewBoundsPolygon, &currentTileMask, &diff);

                    if (diff.num_contours == 0) {
                        completeViewBoundsDrawn = true;
                    }

                    gpc_free_polygon(&diff);
                }
            }

            if(completeViewBoundsDrawn) {
                tileWrapper.isVisible = false;
                it = decltype(it){currentTiles.erase( std::next(it).base() )};
                it = std::prev(it);
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
                tileWrapper.isVisible = false;
                it = decltype(it){currentTiles.erase( std::next(it).base() )};
                it = std::prev(it);
            } else {
                gpc_polygon resultingMask;

                gpc_polygon_clip(GPC_INT, &polygonDiff, &currentViewBoundsPolygon, &resultingMask);

                if (resultingMask.contour == NULL) {
                    tileWrapper.isVisible = false;
                    it = decltype(it){currentTiles.erase( std::next(it).base() )};
                    it = std::prev(it);
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

        if (readyTiles.count(tileInfo) == 0) {
            continue;
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

            freeCurrent = true;
        }
    }

    if(freeCurrent) {
        gpc_free_polygon(&currentTileMask);
    }
    gpc_free_polygon(&currentViewBoundsPolygon);
}

template<class T, class L, class R>
void Tiled2dMapSource<T, L, R>::setTileReady(const Tiled2dMapTileInfo &tile) {
    bool needsUpdate = false;
    
    if (readyTiles.count(tile) == 0) {
        if (currentTiles.count(tile) != 0){
            readyTiles.insert(tile);
            needsUpdate = true;
        }
    }
    
    if (!needsUpdate) { return; }
    
    updateTileMasks();
    
    notifyTilesUpdates();
}

template<class T, class L, class R>
void Tiled2dMapSource<T, L, R>::setTilesReady(const std::vector<const Tiled2dMapTileInfo> &tiles) {
    bool needsUpdate = false;
    
    for (auto const &tile: tiles) {
        if (readyTiles.count(tile) == 0) {
            if (currentTiles.count(tile) != 0){
                readyTiles.insert(tile);
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
    if(notFoundTiles.size() > 0) {
        return LayerReadyState::ERROR;
    }

    for (auto const &[index, errors]: errorTiles) {
        if (errors.size() > 0) {
            return LayerReadyState::ERROR;
        }
    }

    if(!currentlyLoading.empty()) {
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

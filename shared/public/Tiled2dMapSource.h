/*
 * Copyright (c) 2021 Ubique Innovation AG <https://www.ubique.ch>
 *
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this
 *  file, You can obtain one at https://mozilla.org/MPL/2.0/.
 *
 *  SPDX-License-Identifier: MPL-2.0
 */

#pragma once

#include "Coord.h"
#include "CoordinateConversionHelperInterface.h"
#include "LambdaTask.h"
#include "MapConfig.h"
#include "PrioritizedTiled2dMapTileInfo.h"
#include "SchedulerInterface.h"
#include "Tiled2dMapLayerConfig.h"
#include "Tiled2dMapSourceInterface.h"
#include "Tiled2dMapSourceListenerInterface.h"
#include "Tiled2dMapZoomInfo.h"
#include "Tiled2dMapZoomLevelInfo.h"
#include "QuadCoord.h"
#include "CoordinateSystemIdentifiers.h"
#include <atomic>
#include <cmath>
#include <mutex>
#include <set>
#include <unordered_map>
#include <unordered_set>

// T is the Object used for loading
// L is the Loading type
// R is the Result type
template <class T, class L, class R> class Tiled2dMapSource :
    public Tiled2dMapSourceInterface,
    public std::enable_shared_from_this<Tiled2dMapSourceInterface> {
  public:
    Tiled2dMapSource(const MapConfig &mapConfig, const std::shared_ptr<Tiled2dMapLayerConfig> &layerConfig,
                     const std::shared_ptr<CoordinateConversionHelperInterface> &conversionHelper,
                     const std::shared_ptr<SchedulerInterface> &scheduler,
                     const std::shared_ptr<Tiled2dMapSourceListenerInterface> &listener,
                     float screenDensityPpi);

    virtual void onVisibleBoundsChanged(const ::RectCoord &visibleBounds, double zoom) override;

    virtual bool isTileVisible(const Tiled2dMapTileInfo &tileInfo);

    virtual void pause() override;

    virtual void resume() override;

    virtual RectCoord getCurrentViewBounds();

    void setMinZoomLevelIdentifier(std::optional<int32_t> value) override;
    void setMaxZoomLevelIdentifier(std::optional<int32_t> value) override;

    std::optional<int32_t> getMinZoomLevelIdentifier() override;
    std::optional<int32_t> getMaxZoomLevelIdentifier() override;

    virtual ::LayerReadyState isReadyToRenderOffscreen() override;

    virtual void setErrorManager(const std::shared_ptr<::ErrorManager> & errorManager) override;

    virtual void forceReload() override;


  protected:
    virtual L loadTile(Tiled2dMapTileInfo tile) = 0;

    virtual R postLoadingTask(const L &loadedData, const Tiled2dMapTileInfo &tile) = 0;

    MapConfig mapConfig;
    std::shared_ptr<Tiled2dMapLayerConfig> layerConfig;
    std::string layerSystemId;
    std::shared_ptr<CoordinateConversionHelperInterface> conversionHelper;
    std::shared_ptr<SchedulerInterface> scheduler;
    std::weak_ptr<Tiled2dMapSourceListenerInterface> listener;
    std::shared_ptr<::ErrorManager>  errorManager;

    std::vector<Tiled2dMapZoomLevelInfo> zoomLevelInfos;
    const Tiled2dMapZoomInfo zoomInfo;

    std::optional<int32_t> minZoomLevelIdentifier;
    std::optional<int32_t> maxZoomLevelIdentifier;

    std::recursive_mutex tilesMutex;
    std::unordered_map<Tiled2dMapTileInfo, R> currentTiles;
    std::unordered_set<Tiled2dMapTileInfo> currentVisibleTiles;
    RectCoord currentViewBounds = RectCoord(Coord(CoordinateSystemIdentifiers::RENDERSYSTEM(), 0.0, 0.0, 0.0),
                                            Coord(CoordinateSystemIdentifiers::RENDERSYSTEM(), 0.0, 0.0, 0.0));


    std::atomic<bool> isPaused;

    float screenDensityPpi;

  private:
    void updateCurrentTileset(const ::RectCoord &visibleBounds, double zoom);

    void performLoadingTask();

    void onVisibleTilesChanged(const std::unordered_set<PrioritizedTiled2dMapTileInfo> &visibleTiles);

    std::atomic_flag updateFlag = ATOMIC_FLAG_INIT;
    std::atomic_int pendingUpdates = 0;
    std::recursive_mutex updateMutex;
    std::optional<RectCoord> updateBounds;
    std::optional<double> updateZoom;

    std::atomic_size_t dispatchedTasks;
    std::unordered_set<Tiled2dMapTileInfo> currentlyLoading;
    std::unordered_set<PrioritizedTiled2dMapTileInfo> loadingQueue;

    const long long MAX_WAIT_TIME = 32000;
    const long long MIN_WAIT_TIME = 1000;

    struct ErrorInfo {
        long long lastLoad;
        long long delay;
    };

    std::unordered_map<Tiled2dMapTileInfo, ErrorInfo> errorTiles;
    std::unordered_set<Tiled2dMapTileInfo> notFoundTiles;

    std::optional<Tiled2dMapTileInfo> dequeueLoadingTask();
};

#include "Tiled2dMapSourceImpl.h"

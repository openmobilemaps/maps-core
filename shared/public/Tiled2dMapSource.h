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
#include "PolygonCoord.h"
#include "CoordinateSystemIdentifiers.h"
#include <atomic>
#include <cmath>
#include <mutex>
#include <set>
#include <map>
#include <unordered_map>
#include <unordered_set>
#include "gpc.h"

#if(defined __APPLE__ && defined DEBUG)
#include <os/log.h>
#include <os/signpost.h>
#endif

template<class R>
struct TileWrapper {
public:
    const R result;
    std::vector<::PolygonCoord> masks;
    const PolygonCoord tileBounds;
    gpc_polygon tilePolygon;
    bool isVisible = true;

    TileWrapper(const R &result,
                const std::vector<::PolygonCoord> & masks,
                const PolygonCoord & tileBounds,
                const gpc_polygon &tilePolygon) :
    result(std::move(result)),
    masks(std::move(masks)),
    tileBounds(std::move(tileBounds)),
    tilePolygon(std::move(tilePolygon)) {};
};


// T is the Object used for loading
// L is the Loading type
// R is the Result type
template<class T, class L, class R>
class Tiled2dMapSource :
        public Tiled2dMapSourceInterface,
        public std::enable_shared_from_this<Tiled2dMapSourceInterface> {
public:
    Tiled2dMapSource(const MapConfig &mapConfig, const std::shared_ptr<Tiled2dMapLayerConfig> &layerConfig,
                     const std::shared_ptr<CoordinateConversionHelperInterface> &conversionHelper,
                     const std::shared_ptr<SchedulerInterface> &scheduler,
                     const std::shared_ptr<Tiled2dMapSourceListenerInterface> &listener,
                     float screenDensityPpi,
                     size_t loaderCount);

    virtual void onVisibleBoundsChanged(const ::RectCoord &visibleBounds, int curT, double zoom) override;

    virtual bool isTileVisible(const Tiled2dMapTileInfo &tileInfo);

    virtual void pause() override;

    virtual void resume() override;

    virtual RectCoord getCurrentViewBounds();

    void setMinZoomLevelIdentifier(std::optional<int32_t> value) override;

    void setMaxZoomLevelIdentifier(std::optional<int32_t> value) override;

    std::optional<int32_t> getMinZoomLevelIdentifier() override;

    std::optional<int32_t> getMaxZoomLevelIdentifier() override;

    virtual ::LayerReadyState isReadyToRenderOffscreen() override;

    virtual void setErrorManager(const std::shared_ptr<::ErrorManager> &errorManager) override;

    virtual void forceReload() override;

    void setTileReady(const Tiled2dMapTileInfo &tile);

    void setTilesReady(const std::vector<const Tiled2dMapTileInfo> &tiles);

    virtual L loadTile(Tiled2dMapTileInfo tile, size_t loaderIndex) = 0;

  protected:

    virtual R postLoadingTask(const L &loadedData, const Tiled2dMapTileInfo &tile) = 0;

    MapConfig mapConfig;
    std::shared_ptr<Tiled2dMapLayerConfig> layerConfig;
    std::string layerSystemId;
    std::shared_ptr<CoordinateConversionHelperInterface> conversionHelper;
    std::shared_ptr<SchedulerInterface> scheduler;
    std::weak_ptr<Tiled2dMapSourceListenerInterface> listener;
    std::shared_ptr<::ErrorManager> errorManager;

    std::vector<Tiled2dMapZoomLevelInfo> zoomLevelInfos;
    const Tiled2dMapZoomInfo zoomInfo;

    std::optional<int32_t> minZoomLevelIdentifier;
    std::optional<int32_t> maxZoomLevelIdentifier;

    std::recursive_mutex currentTilesMutex;
    std::map<Tiled2dMapTileInfo, TileWrapper<R>> currentTiles;


    std::recursive_mutex currentZoomLevelMutex;
    int currentZoomLevelIdentifier = 0;

    std::recursive_mutex currentVisibleTilesMutex;
    std::unordered_set<Tiled2dMapTileInfo> currentVisibleTiles;

    std::recursive_mutex currentPyramidMutex;
    std::vector<VisibleTilesLayer> currentPyramid;

    RectCoord currentViewBounds = RectCoord(Coord(CoordinateSystemIdentifiers::RENDERSYSTEM(), 0.0, 0.0, 0.0),
                                            Coord(CoordinateSystemIdentifiers::RENDERSYSTEM(), 0.0, 0.0, 0.0));


    std::atomic<bool> isPaused;

    float screenDensityPpi;
    std::recursive_mutex tilesReadyMutex;
    std::set<Tiled2dMapTileInfo> readyTiles;

#if(defined __APPLE__ && defined DEBUG)
    os_log_t logHandle;
    os_signpost_id_t tileLoadingSignPost;
    os_signpost_id_t tilePostLoadingSignPost;
    os_signpost_id_t tileReadySignPost;
#endif

private:
    void updateCurrentTileset(const ::RectCoord &visibleBounds, int curT, double zoom);

    void performLoadingTask(size_t loaderIndex);

    void onVisibleTilesChanged(const std::vector<VisibleTilesLayer> &pyramid);

    void updateTileMasks();

    std::atomic_flag updateFlag = ATOMIC_FLAG_INIT;
    std::atomic_int pendingUpdates = 0;
    std::recursive_mutex updateMutex;
    std::optional<RectCoord> updateBounds;
    std::optional<int> updateT;
    std::optional<double> updateZoom;


    std::recursive_mutex updateTilesetMutex;
    std::recursive_mutex currentlyLoadingMutex;
    std::unordered_set<Tiled2dMapTileInfo> currentlyLoading;

    std::recursive_mutex dispatchedTasksMutex;
    std::unordered_map<size_t, size_t > dispatchedTasks;

        // the key of the map is the loader index, if the first loader returns noop the next one will be used
    std::recursive_mutex loadingQueueMutex;
    std::unordered_map<size_t, std::set<PrioritizedTiled2dMapTileInfo>> loadingQueues;

    const int max_parallel_loading_tasks = 8;
    const long long MAX_WAIT_TIME = 32000;
    const long long MIN_WAIT_TIME = 1000;

    struct ErrorInfo {
        long long lastLoad;
        long long delay;
    };

    std::recursive_mutex errorTilesMutex;
    std::unordered_map<size_t, std::map<PrioritizedTiled2dMapTileInfo, ErrorInfo>> errorTiles;

    std::recursive_mutex notFoundTilesMutex;
    std::unordered_set<Tiled2dMapTileInfo> notFoundTiles;

    std::optional<PrioritizedTiled2dMapTileInfo> dequeueLoadingTask(size_t loaderIndex);
};

#include "Tiled2dMapSourceImpl.h"

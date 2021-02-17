//
// Created by Christoph Maurhofer on 08.02.2021.
//

#pragma once

#include <unordered_set>
#include "Tiled2dMapSourceInterface.h"
#include "Tiled2dMapLayerConfig.h"
#include "SchedulerInterface.h"
#include "Coord.h"
#include "Tiled2dMapSourceListenerInterface.h"
#include "Tiled2dMapZoomLevelInfo.h"
#include "Tiled2dMapZoomInfo.h"
#include "MapConfig.h"
#include "CoordinateConversionHelperInterface.h"
#include "PrioritizedTiled2dMapTileInfo.h"
#include <mutex>
#include <unordered_map>
#include <set>
#include <cmath>
#include <atomic>
#include "LambdaTask.h"

// T is the Object used for loading
// L is the Loading type
template <class T, class L>
class Tiled2dMapSource : public Tiled2dMapSourceInterface {
public:
    Tiled2dMapSource(const MapConfig &mapConfig,
                      const std::shared_ptr<Tiled2dMapLayerConfig> &layerConfig,
                      const std::shared_ptr<CoordinateConversionHelperInterface> &conversionHelper,
                      const std::shared_ptr<SchedulerInterface> &scheduler,
                      const std::shared_ptr<Tiled2dMapSourceListenerInterface> &listener);

    virtual void onVisibleBoundsChanged(const ::RectCoord &visibleBounds, double zoom) override;

    virtual void pause() = 0;

    virtual void resume() = 0;

protected:

    virtual L loadTile(Tiled2dMapTileInfo tile) = 0;

    MapConfig mapConfig;
    std::shared_ptr<Tiled2dMapLayerConfig> layerConfig;
    RectCoord layerBoundsMapSystem;
    std::string layerSystemId;
    std::shared_ptr<CoordinateConversionHelperInterface> conversionHelper;
    std::shared_ptr<SchedulerInterface> scheduler;
    std::shared_ptr<Tiled2dMapSourceListenerInterface> listener;

    std::vector<Tiled2dMapZoomLevelInfo> zoomLevelInfos;
    const Tiled2dMapZoomInfo zoomInfo;

    std::recursive_mutex currentTilesMutex;
    std::unordered_map<Tiled2dMapTileInfo, std::shared_ptr<T>> currentTiles;
    std::unordered_set<Tiled2dMapTileInfo> currentVisibleTiles;

private:
    void updateCurrentTileset(const ::RectCoord &visibleBounds, double zoom);

    void performLoadingTask();

    void onVisibleTilesChanged(const std::unordered_set<PrioritizedTiled2dMapTileInfo> &visibleTiles);

    std::recursive_mutex priorityQueueMutex;
    std::atomic_size_t dispatchedTasks;
    std::unordered_set<Tiled2dMapTileInfo> currentlyLoading;
    std::set<PrioritizedTiled2dMapTileInfo> loadingQueue;

    const long long MAX_WAIT_TIME = 32000;
    const long long MIN_WAIT_TIME = 1000;

    struct ErrorInfo {
        long long lastLoad;
        long long delay;
    };

    std::recursive_mutex errorTilesMutex;
    std::unordered_map<Tiled2dMapTileInfo, ErrorInfo> errorTiles;

    std::recursive_mutex notFoundTilesMutex;
    std::set<Tiled2dMapTileInfo> notFoundTiles;

    std::optional<Tiled2dMapTileInfo> dequeueLoadingTask();
};

#include "Tiled2dMapSourceImpl.h"

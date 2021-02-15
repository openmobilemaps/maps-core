//
// Created by Christoph Maurhofer on 09.02.2021.
//

#pragma once

#include <unordered_map>
#include "TextureLoaderInterface.h"
#include "Tiled2dMapRasterTileInfo.h"
#include "Tiled2dMapSource.h"
#include "MapConfig.h"
#include <mutex>
#include <optional>
#include <set>

class Tiled2dMapRasterSource : public Tiled2dMapSource {
public:
    Tiled2dMapRasterSource(const MapConfig &mapConfig,
                           const std::shared_ptr<Tiled2dMapLayerConfig> &layerConfig,
                           const std::shared_ptr<CoordinateConversionHelperInterface> &conversionHelper,
                           const std::shared_ptr<SchedulerInterface> &scheduler,
                           const std::shared_ptr<TextureLoaderInterface> &loader,
                           const std::shared_ptr<Tiled2dMapSourceListenerInterface> &listener);

    virtual std::unordered_set<Tiled2dMapRasterTileInfo> getCurrentTiles();

    virtual void pause();

    virtual void resume();

protected:
    // tuple of tileInfo ans priority
    virtual void onVisibleTilesChanged(const std::unordered_set<PrioritizedTiled2dMapTileInfo> &visibleTiles);

private:
    const std::shared_ptr<TextureLoaderInterface> loader;

    std::recursive_mutex currentTilesMutex;
    std::unordered_map<Tiled2dMapTileInfo, std::shared_ptr<TextureHolderInterface>> currentTiles;
    std::unordered_set<Tiled2dMapTileInfo> currentVisibleTiles;

    std::recursive_mutex priorityQueueMutex; // used for dispatchedTasks and loadingQueue
    int dispatchedTasks;
    std::set<PrioritizedTiled2dMapTileInfo> loadingQueue;

    std::optional<Tiled2dMapTileInfo> dequeueLoadingTask();
    void performLoadingTask();
};

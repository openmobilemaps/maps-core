//
// Created by Christoph Maurhofer on 09.02.2021.
//

#include "Tiled2dMapRasterSource.h"
#include <string>
#include <algorithm>
#include "LambdaTask.h"
#include <cmath>
#include "TextureLoaderResult.h"

Tiled2dMapRasterSource::Tiled2dMapRasterSource(const MapConfig &mapConfig,
                                               const std::shared_ptr<Tiled2dMapLayerConfig> &layerConfig,
                                               const std::shared_ptr<CoordinateConversionHelperInterface> &conversionHelper,
                                               const std::shared_ptr<SchedulerInterface> &scheduler,
                                               const std::shared_ptr<TextureLoaderInterface> &loader,
                                               const std::shared_ptr<Tiled2dMapSourceListenerInterface> &listener)
: Tiled2dMapSource(mapConfig, layerConfig, conversionHelper, scheduler, listener),
loader(loader) {
}

void Tiled2dMapRasterSource::onVisibleTilesChanged(const std::unordered_set<PrioritizedTiled2dMapTileInfo> &visibleTiles) {
    std::lock_guard<std::recursive_mutex> lock(currentTilesMutex);

    currentVisibleTiles.clear();

    std::unordered_set<PrioritizedTiled2dMapTileInfo> toAdd;
    for (const auto &tileInfo: visibleTiles) {
        currentVisibleTiles.insert(tileInfo.tileInfo);

        if (currentTiles.count(tileInfo.tileInfo) == 0) {
            toAdd.insert(tileInfo);
        }
    }

    std::unordered_set<Tiled2dMapTileInfo> toRemove;
    for (const auto &tileEntry: currentTiles) {
        bool found = false;
        for (const auto &tile: visibleTiles) {
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

    {
        std::lock_guard<std::recursive_mutex> overlayLock(priorityQueueMutex);

        for (auto it = loadingQueue.begin(); it != loadingQueue.end(); ) {
            if (visibleTiles.count(*it) == 0) {
                it = loadingQueue.erase(it);
            }
            else {
                ++it;
            }
        }

        for (const auto &addedTile : toAdd) {

            {
                std::lock_guard<std::recursive_mutex> overlayLock(priorityQueueMutex);
                if (loadingQueue.count(addedTile) == 0) {
                    loadingQueue.insert(addedTile);
                }
            }
        }

        size_t tasksToAdd = loadingQueue.size() > dispatchedTasks ? loadingQueue.size() - dispatchedTasks : 0;
        for (int taskCount = 0;  taskCount < tasksToAdd; taskCount++) {
            auto taskIdentifier = "Tiled2dMapRasterSource_loadTile_" + std::to_string(taskCount);
            scheduler->addTask(std::make_shared<LambdaTask>(
                                                            TaskConfig(taskIdentifier,
                                                                       0,
                                                                       TaskPriority::NORMAL,
                                                                       ExecutionEnvironment::IO),
                                                            [=] { performLoadingTask(); }));
            dispatchedTasks += 1;
        }
    }

    //listener->onTilesUpdated();
}

std::unordered_set<Tiled2dMapRasterTileInfo> Tiled2dMapRasterSource::getCurrentTiles() {
    std::lock_guard<std::recursive_mutex> lock(currentTilesMutex);
    std::unordered_set<Tiled2dMapRasterTileInfo> currentTileInfos;
    for (const auto &tileEntry: currentTiles) {
        currentTileInfos.insert(Tiled2dMapRasterTileInfo(tileEntry.first, tileEntry.second));
    }
    return currentTileInfos;
}

void Tiled2dMapRasterSource::pause() {
    // TODO: Stop loading tiles
}

void Tiled2dMapRasterSource::resume() {
    // TODO: Reload textures of current tiles
}


std::optional<Tiled2dMapTileInfo> Tiled2dMapRasterSource::dequeueLoadingTask(){
    std::lock_guard<std::recursive_mutex> lock(priorityQueueMutex);

    dispatchedTasks -= 1;

    if (loadingQueue.empty()) {
        return std::nullopt;
    }

    auto tile = loadingQueue.begin();

    loadingQueue.erase(tile);

    return tile->tileInfo;
}

void Tiled2dMapRasterSource::performLoadingTask() {
    if (auto tile = dequeueLoadingTask()) {
        auto loaderResult = loader->loadTexture(layerConfig->getTileUrl(tile->x, tile->y, tile->zoom));


        if ( loaderResult.textureHolder ){
            std::lock_guard<std::recursive_mutex> lock(currentTilesMutex);
            if (currentVisibleTiles.count(*tile)) {
                currentTiles[*tile] = loaderResult.textureHolder;
            }
        } else {
            // TODO: handle error eg. exp. backoff
        }
        
        listener->onTilesUpdated();
    }
}

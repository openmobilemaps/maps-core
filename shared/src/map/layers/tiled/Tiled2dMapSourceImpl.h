//
// Created by Christoph Maurhofer on 08.02.2021.
//


template <class T>
Tiled2dMapSource<T>::Tiled2dMapSource(const MapConfig &mapConfig,
                                       const std::shared_ptr<Tiled2dMapLayerConfig> &layerConfig,
                                       const std::shared_ptr<CoordinateConversionHelperInterface> &conversionHelper,
                                       const std::shared_ptr<SchedulerInterface> &scheduler,
                                       const std::shared_ptr<Tiled2dMapSourceListenerInterface> &listener)
        : mapConfig(mapConfig),
          layerConfig(layerConfig),
          conversionHelper(conversionHelper),
          scheduler(scheduler),
          listener(listener),
          zoomInfo(layerConfig->getZoomLevelInfos()),
          layerBoundsMapSystem(conversionHelper->convertRect(mapConfig.mapCoordinateSystem.identifier, layerConfig->getBounds())),
          layerSystemId(layerConfig->getBounds().topLeft.systemIdentifier) {
}

template <class T>
void Tiled2dMapSource<T>::onVisibleBoundsChanged(const ::RectCoord &visibleBounds, double zoom) {
    scheduler->addTask(std::make_shared<LambdaTask>(
            TaskConfig("Tiled2dMapSource_Update",
                       0,
                       TaskPriority::NORMAL,
                       ExecutionEnvironment::COMPUTATION),
            [=] { updateCurrentTileset(visibleBounds, zoom); }));
}

template <class T>
void Tiled2dMapSource<T>::updateCurrentTileset(const RectCoord &visibleBounds, double zoom) {
    std::unordered_set<PrioritizedTiled2dMapTileInfo> visibleTiles;

    RectCoord layerBounds = layerConfig->getBounds();
    RectCoord visibleBoundsLayer = conversionHelper->convertRect(layerSystemId, visibleBounds);

    double centerVisibleX = visibleBoundsLayer.topLeft.x + 0.5 * (visibleBoundsLayer.bottomRight.x - visibleBoundsLayer.topLeft.x);
    double centerVisibleY = visibleBoundsLayer.topLeft.y + 0.5 * (visibleBoundsLayer.bottomRight.y - visibleBoundsLayer.topLeft.y);

    int zoomInd = 0;
    int zoomPriorityRange = 20;
    for (const Tiled2dMapZoomLevelInfo &zoomLevelInfo : zoomInfo) {
        if (zoomLevelInfo.zoom < zoom) {

            double tileWidth = zoomLevelInfo.tileWidthLayerSystemUnits;

            bool leftToRight = layerBounds.topLeft.x < layerBounds.bottomRight.x;
            bool topToBottom = layerBounds.topLeft.y < layerBounds.bottomRight.y;
            double tileWidthAdj = leftToRight ? tileWidth : -tileWidth;
            double tileHeightAdj = topToBottom ? tileWidth : -tileWidth;

            double visibleLeft = visibleBoundsLayer.topLeft.x;
            double visibleRight = visibleBoundsLayer.bottomRight.x;
            double visibleWidth = std::abs(visibleLeft - visibleRight);
            double boundsLeft = layerBounds.topLeft.x;
            int startTileLeft = std::floor(std::max(leftToRight ? (visibleLeft - boundsLeft) : (boundsLeft - visibleLeft), 0.0) / tileWidth);
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
                    double tileCenterDis = std::sqrt(
                            std::pow(tileCenterX - centerVisibleX, 2.0) + std::pow(tileCenterY - centerVisibleY, 2.0));

                    visibleTiles.insert(
                            PrioritizedTiled2dMapTileInfo(Tiled2dMapTileInfo(rect, x, y, zoomLevelInfo.zoomLevelIdentifier),
                                                          std::ceil((tileCenterDis / maxDisCenter) * zoomPriorityRange) +
                                                          zoomInd * zoomPriorityRange));
                }
            }

            zoomInd++;
            break;
        }

    }

    onVisibleTilesChanged(visibleTiles);
}

template <class T>
void Tiled2dMapSource<T>::onVisibleTilesChanged(const std::unordered_set<PrioritizedTiled2dMapTileInfo> &visibleTiles) {
    std::lock_guard<std::recursive_mutex> lock(currentTilesMutex);

    currentVisibleTiles.clear();

    std::unordered_set<PrioritizedTiled2dMapTileInfo> toAdd;
    for (const auto &tileInfo: visibleTiles) {
        currentVisibleTiles.insert(tileInfo.tileInfo);

        if (currentTiles.count(tileInfo.tileInfo) == 0 && currentlyLoading.count(tileInfo.tileInfo) == 0) {
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
            auto taskIdentifier = "Tiled2dMapSource_loadingTask" + std::to_string(taskCount);
            scheduler->addTask(std::make_shared<LambdaTask>(
                                                            TaskConfig(taskIdentifier,
                                                                       0,
                                                                       TaskPriority::NORMAL,
                                                                       ExecutionEnvironment::IO),
                                                            [=] { performLoadingTask(); }));
            dispatchedTasks += 1;
        }
    }

    listener->onTilesUpdated();
}

template <class T>
std::optional<Tiled2dMapTileInfo> Tiled2dMapSource<T>::dequeueLoadingTask(){
    std::lock_guard<std::recursive_mutex> lock(priorityQueueMutex);

    dispatchedTasks -= 1;

    if (loadingQueue.empty()) {
        return std::nullopt;
    }

    auto tile = loadingQueue.begin();
    auto tileInfo = tile->tileInfo;

    loadingQueue.erase(tile);

    currentlyLoading.insert(tileInfo);

    return tileInfo;
}

template <class T>
void Tiled2dMapSource<T>::performLoadingTask() {
    if (auto tile = dequeueLoadingTask()) {

        loadTile(*tile);

        {
            std::lock_guard<std::recursive_mutex> lock(priorityQueueMutex);
            currentlyLoading.erase(*tile);
        }

        listener->onTilesUpdated();
    }
}


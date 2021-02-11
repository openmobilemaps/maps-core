//
// Created by Christoph Maurhofer on 08.02.2021.
//

#include <cmath>
#include "LambdaTask.h"
#include "Tiled2dMapSource.h"

Tiled2dMapSource::Tiled2dMapSource(const MapConfig &mapConfig,
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
          layerSystemIdentifier(layerConfig->getBounds().topLeft.systemIdentifier) {
}

void Tiled2dMapSource::onVisibleBoundsChanged(const ::RectCoord &visibleBounds, double zoom) {
    scheduler->addTask(std::make_shared<LambdaTask>(
            TaskConfig("Tiled2dMapSource_Update",
                       0,
                       TaskPriority::NORMAL,
                       ExecutionEnvironment::COMPUTATION),
            [=] { updateCurrentTileset(visibleBounds, zoom); }));
}

void Tiled2dMapSource::updateCurrentTileset(const RectCoord &visibleBounds, double zoom) {
    // TODO: update current tileset -> call onVisibleTilesChanged
    std::unordered_set<Tiled2dMapTileInfo> visibleTiles;

    RectCoord layerBounds = layerConfig->getBounds();
    RectCoord visibleBoundsLayer = conversionHelper->convertRect(layerSystemIdentifier, visibleBounds);

    // for(über alle relevanten layers) { hinzufügen von sichtbaren tiles }

    for (const Tiled2dMapZoomLevelInfo &zoomLevelInfo : zoomInfo) {
        if (zoomLevelInfo.zoom < zoom) {

            // TODO: solve correct for other systems (i.e. top.y < bottom.y)
            double tileWidth = zoomLevelInfo.tileWidthLayerSystemUnits;

            double visibleLeft = visibleBoundsLayer.topLeft.x;
            double boundsLeft = layerBounds.topLeft.x;
            int startTileLeft = std::floor(std::max(visibleLeft - boundsLeft, 0.0) / tileWidth);
            double visibleBottom = visibleBoundsLayer.bottomRight.y;
            double boundsBottom = layerBounds.bottomRight.y;
            int startTileBottom = std::floor(std::max(visibleBottom - boundsBottom, 0.0) / tileWidth);

            for (int x = startTileLeft; x * tileWidth + boundsLeft <= std::min(visibleBoundsLayer.bottomRight.x, layerBounds.bottomRight.x); x++) {
                for (int y = startTileBottom; y * tileWidth + boundsBottom <= std::min(visibleBoundsLayer.topLeft.y, layerBounds.topLeft.y); y++) {
                    Coord tileTopLeft = Coord(layerSystemIdentifier, x * tileWidth + boundsLeft, y * tileWidth + boundsBottom, 0);
                    Coord tileBottomRight = Coord(layerSystemIdentifier, tileTopLeft.x + tileWidth, tileTopLeft.y + tileWidth, 0);
                    // TODO: Set priority to useful value instead of '1' (e.g. weighted by distance to center)
                    visibleTiles.insert(Tiled2dMapTileInfo(RectCoord(tileTopLeft, tileBottomRight),
                                                           x, y, zoomLevelInfo.zoomLevelIdentifier,
                                                           1));
                }
            }
            break;
        }
    }

    onVisibleTilesChanged(visibleTiles);
}


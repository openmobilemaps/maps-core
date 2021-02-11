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
          layerSystemId(layerConfig->getBounds().topLeft.systemIdentifier) {
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
    RectCoord visibleBoundsLayer = conversionHelper->convertRect(layerSystemId, visibleBounds);

    // for(über alle relevanten layers) { hinzufügen von sichtbaren tiles }

    for (const Tiled2dMapZoomLevelInfo &zoomLevelInfo : zoomInfo) {
        if (zoomLevelInfo.zoom < zoom) {

            double tileWidth = zoomLevelInfo.tileWidthLayerSystemUnits;

            bool leftToRight = layerBounds.topLeft.x < layerBounds.bottomRight.x;
            bool topToBottom = layerBounds.topLeft.y < layerBounds.bottomRight.y;
            double tileWidthAdj = leftToRight ? tileWidth : -tileWidth;
            double tileHeightAdj = topToBottom ? tileWidth : -tileWidth;

            double visibleLeft = visibleBoundsLayer.topLeft.x;
            double visibleRight = visibleBoundsLayer.bottomRight.x;
            double boundsLeft = layerBounds.topLeft.x;
            int startTileLeft = std::floor(std::abs(visibleLeft - boundsLeft) / tileWidth);
            int maxTileLeft = std::ceil(std::abs(visibleLeft - visibleRight) / tileWidth) + startTileLeft;
            double visibleTop = visibleBoundsLayer.topLeft.y;
            double visibleBottom = visibleBoundsLayer.bottomRight.y;
            double boundsTop = layerBounds.topLeft.y;
            int startTileTop = std::floor(std::abs(visibleTop - boundsTop) / tileWidth);
            int maxTileTop = std::ceil(std::abs(visibleTop - visibleBottom) / tileWidth) + startTileTop;

            for (int x = startTileLeft; x <= maxTileLeft && x < zoomLevelInfo.numTilesX; x++) {
                for (int y = startTileTop; y <= maxTileTop && y < zoomLevelInfo.numTilesY; y++) {
                    Coord tileTopLeft = Coord(layerSystemId, x * tileWidthAdj + boundsLeft, y * tileHeightAdj + boundsTop, 0);
                    Coord tileBottomRight = Coord(layerSystemId, tileTopLeft.x + tileWidthAdj, tileTopLeft.y + tileHeightAdj, 0);

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
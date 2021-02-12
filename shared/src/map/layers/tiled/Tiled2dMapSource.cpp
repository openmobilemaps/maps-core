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

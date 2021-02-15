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
#include "MapConfig.h"
#include "CoordinateConversionHelperInterface.h"
#include "PrioritizedTiled2dMapTileInfo.h"

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
    virtual void onVisibleTilesChanged(const std::unordered_set<PrioritizedTiled2dMapTileInfo> &visibleTiles) = 0;

    MapConfig mapConfig;
    std::shared_ptr<Tiled2dMapLayerConfig> layerConfig;
    RectCoord layerBoundsMapSystem;
    std::string layerSystemId;
    std::shared_ptr<CoordinateConversionHelperInterface> conversionHelper;
    std::shared_ptr<SchedulerInterface> scheduler;
    std::shared_ptr<Tiled2dMapSourceListenerInterface> listener;

    const std::vector<Tiled2dMapZoomLevelInfo> zoomInfo;

private:
    void updateCurrentTileset(const ::RectCoord &visibleBounds, double zoom);
};

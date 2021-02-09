//
// Created by Christoph Maurhofer on 08.02.2021.
//

#pragma once

#include <set>
#include "Tiled2dMapSourceInterface.h"
#include "Tiled2dMapLayerConfig.h"
#include "SchedulerInterface.h"
#include "Coord.h"
#include "Tiled2dMapTileInfo.h"
#include "Tiled2dMapSourceListenerInterface.h"

class Tiled2dMapSource : public Tiled2dMapSourceInterface {
public:
    Tiled2dMapSource(const std::shared_ptr<Tiled2dMapLayerConfig> &layerConfig,
                     const std::shared_ptr<SchedulerInterface> &scheduler,
                     const std::shared_ptr<Tiled2dMapSourceListenerInterface> &listener);

    virtual void onVisibleBoundsChanged(const ::RectCoord &visibleBounds, double zoom);

protected:
    virtual void onVisibleTilesChanged(const std::set<Tiled2dMapTileInfo> &visibleTiles) = 0;

    std::shared_ptr<Tiled2dMapLayerConfig> layerConfig;
    std::shared_ptr<SchedulerInterface> scheduler;
    std::shared_ptr<Tiled2dMapSourceListenerInterface> listener;

    const std::vector<Tiled2dMapZoomLevelInfo> zoomInfo;

private:
    void updateCurrentTileset(const ::RectCoord &visibleBounds, double zoom);
};
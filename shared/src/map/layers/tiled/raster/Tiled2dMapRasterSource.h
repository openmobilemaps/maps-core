//
// Created by Christoph Maurhofer on 09.02.2021.
//

#pragma once

#include <map>
#include "TextureLoaderInterface.h"
#include "Tiled2dMapRasterTileInfo.h"
#include "Tiled2dMapSource.h"
#include "MapConfig.h"

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
    virtual void onVisibleTilesChanged(const std::unordered_set<Tiled2dMapTileInfo> &visibleTiles);

private:
    const std::shared_ptr<TextureLoaderInterface> loader;

    std::map<Tiled2dMapTileInfo, std::shared_ptr<TextureHolderInterface>> currentTiles;
};



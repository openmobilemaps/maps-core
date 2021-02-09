//
// Created by Christoph Maurhofer on 09.02.2021.
//

#pragma once

#include <map>
#include "TextureLoaderInterface.h"
#include "Tiled2dMapSource.h"

class Tiled2dMapRasterSource : public Tiled2dMapSource {
public:
    Tiled2dMapRasterSource(const std::shared_ptr<Tiled2dMapLayerConfig> &layerConfig,
                           const std::shared_ptr<SchedulerInterface> &scheduler,
                           const std::shared_ptr<TextureLoaderInterface> &loader,
                           const std::shared_ptr<Tiled2dMapSourceListenerInterface> &listener);

protected:
    virtual void onVisibleTilesChanged(const std::set<Tiled2dMapTileInfo> &visibleTiles);

private:
    const std::shared_ptr<TextureLoaderInterface> loader;

    std::map<Tiled2dMapTileInfo, std::shared_ptr<TextureHolderInterface>> currentTiles;
};



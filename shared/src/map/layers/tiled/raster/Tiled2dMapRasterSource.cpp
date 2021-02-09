//
// Created by Christoph Maurhofer on 09.02.2021.
//

#include "Tiled2dMapRasterSource.h"

Tiled2dMapRasterSource::Tiled2dMapRasterSource(const std::shared_ptr<Tiled2dMapLayerConfig> &layerConfig,
                                               const std::shared_ptr<SchedulerInterface> &scheduler,
                                               const std::shared_ptr<TextureLoaderInterface> &loader,
                                               const std::shared_ptr<Tiled2dMapSourceListenerInterface> &listener)
        : Tiled2dMapSource(layerConfig, scheduler, listener),
          loader(loader) {
}

void Tiled2dMapRasterSource::onVisibleTilesChanged(const std::set<Tiled2dMapTileInfo> &visibleTiles) {
    // TODO: Check difference in currentSet/newVisible tiles, load new ones, inform the listener via listener->onTilesUpdated()
}
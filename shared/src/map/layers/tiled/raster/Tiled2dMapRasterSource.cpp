//
// Created by Christoph Maurhofer on 09.02.2021.
//

#include "Tiled2dMapRasterSource.h"
#include <string>
#include <algorithm>

Tiled2dMapRasterSource::Tiled2dMapRasterSource(const MapConfig &mapConfig,
                                               const std::shared_ptr<Tiled2dMapLayerConfig> &layerConfig,
                                               const std::shared_ptr<CoordinateConversionHelperInterface> &conversionHelper,
                                               const std::shared_ptr<SchedulerInterface> &scheduler,
                                               const std::shared_ptr<TextureLoaderInterface> &loader,
                                               const std::shared_ptr<Tiled2dMapSourceListenerInterface> &listener)
        : Tiled2dMapSource(mapConfig, layerConfig, conversionHelper, scheduler, listener),
          loader(loader) {
}

void Tiled2dMapRasterSource::onVisibleTilesChanged(const std::unordered_set<Tiled2dMapTileInfo> &visibleTiles) {
    //TODO: check if it okay to not handle all touch inputs
    std::unique_lock<std::recursive_mutex> lock(currentTilesMutex, std::try_to_lock);
    if(!lock.owns_lock()) {
      // update tiles is already happening
      return;
    }

    // TODO: Check difference in currentSet/newVisible tiles, load new ones, inform the listener via listener->onTilesUpdated()

    std::unordered_set<Tiled2dMapTileInfo> toAdd;
    for (const auto &tileInfo: visibleTiles) {
        if (!currentTiles[tileInfo]) toAdd.insert(tileInfo);
    }

    std::unordered_set<Tiled2dMapTileInfo> toRemove;
    for (const auto &tileEntry: currentTiles) {
        if (visibleTiles.count(tileEntry.first) == 0) toRemove.insert(tileEntry.first);
    }

    // TODO: load new tiles, remove the removed ones

    for (const auto &removedTile : toRemove) {
        currentTiles.erase(removedTile);
    }

    for (const auto &addedTile : toAdd) {
      currentTiles[addedTile] = loader->loadTexture(layerConfig->getTileUrl(addedTile.x, addedTile.y, addedTile.zoom));
    }

    listener->onTilesUpdated();
}

std::unordered_set<Tiled2dMapRasterTileInfo> Tiled2dMapRasterSource::getCurrentTiles() {
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

//
// Created by Christoph Maurhofer on 09.02.2021.
//

#include "Tiled2dMapRasterSource.h"

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
      std::string url = "https://wmts100.geo.admin.ch/1.0.0/ch.swisstopo.pixelkarte-farbe/default/current/2056/" + std::to_string(addedTile.zoom) + "/" + std::to_string(addedTile.x) + "/" + std::to_string(addedTile.y) + ".jpeg";
      currentTiles[addedTile] = loader->loadTexture(url);
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

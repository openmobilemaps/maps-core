//
// Created by Christoph Maurhofer on 09.02.2021.
//

#include "Tiled2dMapRasterSource.h"
#include "LambdaTask.h"
#include <algorithm>
#include <cmath>
#include <string>

Tiled2dMapRasterSource::Tiled2dMapRasterSource(const MapConfig &mapConfig,
                                               const std::shared_ptr<Tiled2dMapLayerConfig> &layerConfig,
                                               const std::shared_ptr<CoordinateConversionHelperInterface> &conversionHelper,
                                               const std::shared_ptr<SchedulerInterface> &scheduler,
                                               const std::shared_ptr<TextureLoaderInterface> &loader,
                                               const std::shared_ptr<Tiled2dMapSourceListenerInterface> &listener)
    : Tiled2dMapSource<TextureHolderInterface, TextureLoaderResult>(mapConfig, layerConfig, conversionHelper, scheduler, listener)
    , loader(loader) {}

TextureLoaderResult Tiled2dMapRasterSource::loadTile(Tiled2dMapTileInfo tile) {
    return loader->loadTexture(layerConfig->getTileUrl(tile.x, tile.y, tile.zoomIdentifier));
}

std::unordered_set<Tiled2dMapRasterTileInfo> Tiled2dMapRasterSource::getCurrentTiles() {
    std::lock_guard<std::recursive_mutex> lock(currentTilesMutex);
    std::unordered_set<Tiled2dMapRasterTileInfo> currentTileInfos;
    for (const auto &tileEntry : currentTiles) {
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

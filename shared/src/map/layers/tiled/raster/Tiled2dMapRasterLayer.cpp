//
// Created by Christoph Maurhofer on 09.02.2021.
//

#include "Tiled2dMapRasterLayer.h"

Tiled2dMapRasterLayer::Tiled2dMapRasterLayer(const std::shared_ptr<::MapInterface> &mapInterface,
                                             const std::shared_ptr<::Tiled2dMapLayerConfig> &layerConfig,
                                             const std::shared_ptr<::TextureLoaderInterface> &textureLoader)
        : source(std::make_shared<Tiled2dMapRasterSource>(layerConfig, mapInterface->getScheduler(), textureLoader, shared_from_this())),
          Tiled2dMapLayer(mapInterface, layerConfig, source) {
}

std::shared_ptr<::LayerInterface> Tiled2dMapRasterLayer::asLayerInterface() {
    return shared_from_this();
}

std::vector<std::shared_ptr<::RenderPassInterface>> Tiled2dMapRasterLayer::buildRenderPasses() {
    return std::vector<std::shared_ptr<::RenderPassInterface>>();
    // TODO: Collect info from all layer objects
}

std::string Tiled2dMapRasterLayer::getIdentifier() {
    return "RasterTileLayer"; // TODO: Fix identifier (e.g. include layerConfig-Info)
}

void Tiled2dMapRasterLayer::pause() {
    Tiled2dMapLayer::pause();
    // TODO: Handle layer objects (i.e. clear)
}

void Tiled2dMapRasterLayer::resume() {
    Tiled2dMapLayer::resume();
    // TODO: Handle layer objects (i.e. setup)
}

void Tiled2dMapRasterLayer::onTilesUpdated() {
    // TODO: Check for current tiles in source and create corresponding layer objects (resp. remove old ones)
}



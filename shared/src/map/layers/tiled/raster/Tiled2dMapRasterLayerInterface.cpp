//
// Created by Christoph Maurhofer on 09.02.2021.
//

#include "Tiled2dMapRasterLayerInterface.h"
#include "Tiled2dMapRasterLayer.h"


std::shared_ptr <Tiled2dMapRasterLayerInterface>
Tiled2dMapRasterLayerInterface::create(const std::shared_ptr<::MapInterface> &mapInterface,
                                       const std::shared_ptr<::Tiled2dMapLayerConfig> &layerConfig,
                                       const std::shared_ptr<::TextureLoaderInterface> &textureLoader) {
    return std::make_shared<Tiled2dMapRasterLayer>(mapInterface, layerConfig, textureLoader);
}

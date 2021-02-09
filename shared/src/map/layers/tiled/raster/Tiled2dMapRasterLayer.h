//
// Created by Christoph Maurhofer on 09.02.2021.
//

#pragma once


#include "Tiled2dMapLayer.h"
#include "Tiled2dMapRasterLayerInterface.h"
#include "LayerInterface.h"
#include "Tiled2dMapRasterSource.h"

class Tiled2dMapRasterLayer
        : public Tiled2dMapLayer, public Tiled2dMapRasterLayerInterface, std::enable_shared_from_this<Tiled2dMapRasterLayer> {
public:
    Tiled2dMapRasterLayer(const std::shared_ptr<::MapInterface> &mapInterface,
                          const std::shared_ptr<::Tiled2dMapLayerConfig> &layerConfig,
                          const std::shared_ptr<::TextureLoaderInterface> &textureLoader);

    virtual std::shared_ptr<::LayerInterface> asLayerInterface();

    virtual std::vector<std::shared_ptr<::RenderPassInterface>> buildRenderPasses();

    virtual std::string getIdentifier();

    virtual void pause();

    virtual void resume();

    virtual void onTilesUpdated();

private:
    std::shared_ptr<Tiled2dMapRasterSource> source;
};

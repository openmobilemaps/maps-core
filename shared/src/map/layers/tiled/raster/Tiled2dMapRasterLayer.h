//
// Created by Christoph Maurhofer on 09.02.2021.
//

#pragma once


#include <unordered_map>
#include <mutex>
#include "Tiled2dMapLayer.h"
#include "Tiled2dMapRasterLayerInterface.h"
#include "LayerInterface.h"
#include "Tiled2dMapRasterSource.h"
#include "Textured2dLayerObject.h"

class Tiled2dMapRasterLayer
        : public Tiled2dMapLayer, public Tiled2dMapRasterLayerInterface {
public:
    Tiled2dMapRasterLayer(const std::shared_ptr<::MapInterface> &mapInterface,
                          const std::shared_ptr<::Tiled2dMapLayerConfig> &layerConfig,
                          const std::shared_ptr<::TextureLoaderInterface> &textureLoader);

    virtual void onAdded() override;

    virtual void onRemoved() override;

    virtual std::shared_ptr<::LayerInterface> asLayerInterface() override;

    virtual std::vector<std::shared_ptr<::RenderPassInterface>> buildRenderPasses() override;

    virtual std::string getIdentifier() override;

    virtual void pause() override;

    virtual void resume() override;

    virtual void onTilesUpdated() override;

private:
    std::shared_ptr<TextureLoaderInterface> textureLoader;
    std::shared_ptr<Tiled2dMapRasterSource> rasterSource;

    std::recursive_mutex updateMutex;
    std::unordered_map<Tiled2dMapRasterTileInfo, std::shared_ptr<Textured2dLayerObject>> tileObjectMap;
    std::vector<std::shared_ptr<RenderPassInterface>> renderPasses;
          
    std::shared_ptr<AlphaShaderInterface> alphaShader;
};

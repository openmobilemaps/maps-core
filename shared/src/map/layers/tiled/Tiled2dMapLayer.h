#pragma once

#include "MapInterface.h"
#include "RenderPassInterface.h"
#include "LayerInterface.h"
#include "Tiled2dMapSourceInterface.h"
#include "Tiled2dMapLayerInterface.h"
#include "Tiled2dMapLayerConfig.h"

class Tiled2dMapLayer : public Tiled2dMapLayerInterface, std::enable_shared_from_this<LayerInterface> {
public:

    Tiled2dMapLayer(const std::shared_ptr<MapInterface> &mapInterface, const std::shared_ptr<Tiled2dMapLayerConfig> &layerConfig);

    virtual std::shared_ptr<LayerInterface> asLayerInterface();

    virtual std::vector<std::shared_ptr<::RenderPassInterface>> buildRenderPasses();

    virtual std::string getIdentifier();

    virtual void pause();

    virtual void resume();

    virtual void hide();

    virtual void show();

private:
    const std::shared_ptr<MapInterface> mapInterface;
    const std::shared_ptr<Tiled2dMapLayerConfig> layerConfig;
    std::shared_ptr<Tiled2dMapSourceInterface> source;
};

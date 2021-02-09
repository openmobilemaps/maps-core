#pragma once

#include "MapInterface.h"
#include "RenderPassInterface.h"
#include "LayerInterface.h"
#include "Tiled2dMapSourceInterface.h"
#include "Tiled2dMapLayerConfig.h"
#include "Tiled2dMapSourceListenerInterface.h"
#include "MapCamera2dListenerInterface.h"

class Tiled2dMapLayer : public LayerInterface, public Tiled2dMapSourceListenerInterface, public MapCamera2dListenerInterface {
public:

    Tiled2dMapLayer(const std::shared_ptr<MapInterface> &mapInterface, const std::shared_ptr<Tiled2dMapLayerConfig> &layerConfig,
                    const std::shared_ptr<Tiled2dMapSourceInterface> &source);

    virtual std::vector<std::shared_ptr<::RenderPassInterface>> buildRenderPasses() = 0;

    virtual std::string getIdentifier() = 0;

    virtual void pause();

    virtual void resume();

    virtual void hide();

    virtual void show();

    virtual void onTilesUpdated() = 0;

    virtual void onVisibleBoundsChanged(const ::RectCoord &visibleBounds, double zoom);

protected:
    const std::shared_ptr<MapInterface> mapInterface;
    const std::shared_ptr<Tiled2dMapLayerConfig> layerConfig;
    std::shared_ptr<Tiled2dMapSourceInterface> source;

    bool isHidden = false;
};

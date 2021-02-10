#pragma once

#include "MapInterface.h"
#include "RenderPassInterface.h"
#include "LayerInterface.h"
#include "Tiled2dMapSourceInterface.h"
#include "Tiled2dMapLayerConfig.h"
#include "Tiled2dMapSourceListenerInterface.h"
#include "MapCamera2dListenerInterface.h"

class Tiled2dMapLayer : public LayerInterface, public Tiled2dMapSourceListenerInterface, public MapCamera2dListenerInterface,
                        public std::enable_shared_from_this<Tiled2dMapLayer> {
public:

    Tiled2dMapLayer(const std::shared_ptr<MapInterface> &mapInterface, const std::shared_ptr<Tiled2dMapLayerConfig> &layerConfig);

    void setSourceInterface(const std::shared_ptr<Tiled2dMapSourceInterface> &sourceInterface);

    virtual std::vector<std::shared_ptr<::RenderPassInterface>> buildRenderPasses() = 0;

    virtual std::string getIdentifier() = 0;

    virtual void onAdded();

    virtual void onRemoved();

    virtual void pause() = 0;

    virtual void resume() = 0;

    virtual void hide();

    virtual void show();

    virtual void onTilesUpdated() = 0;

    virtual void onVisibleBoundsChanged(const ::RectCoord &visibleBounds, double zoom);

protected:
    const std::shared_ptr<MapInterface> mapInterface;
    const std::shared_ptr<Tiled2dMapLayerConfig> layerConfig;
    std::shared_ptr<Tiled2dMapSourceInterface> sourceInterface;

    bool isHidden = false;
};

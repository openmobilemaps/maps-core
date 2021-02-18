#pragma once

#include "LayerInterface.h"
#include "MapCamera2dListenerInterface.h"
#include "MapInterface.h"
#include "RenderPassInterface.h"
#include "Tiled2dMapLayerConfig.h"
#include "Tiled2dMapSourceInterface.h"
#include "Tiled2dMapSourceListenerInterface.h"

class Tiled2dMapLayer : public LayerInterface,
                        public Tiled2dMapSourceListenerInterface,
                        public MapCamera2dListenerInterface,
                        public std::enable_shared_from_this<Tiled2dMapLayer> {
  public:
    Tiled2dMapLayer(const std::shared_ptr<Tiled2dMapLayerConfig> &layerConfig);

    void setSourceInterface(const std::shared_ptr<Tiled2dMapSourceInterface> &sourceInterface);

    virtual void update() override = 0;

    virtual std::vector<std::shared_ptr<::RenderPassInterface>> buildRenderPasses() override = 0;

    virtual void onAdded(const std::shared_ptr<::MapInterface> &mapInterface) override;

    virtual void onRemoved() override;

    virtual void pause() override = 0;

    virtual void resume() override = 0;

    virtual void hide() override;

    virtual void show() override;

    virtual void onTilesUpdated() override = 0;

    virtual void onVisibleBoundsChanged(const ::RectCoord &visibleBounds, double zoom) override;

  protected:
    std::shared_ptr<MapInterface> mapInterface;
    const std::shared_ptr<Tiled2dMapLayerConfig> layerConfig;
    std::shared_ptr<Tiled2dMapSourceInterface> sourceInterface;

    bool isHidden = false;
};

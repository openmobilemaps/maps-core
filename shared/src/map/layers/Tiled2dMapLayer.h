#pragma once
#include "LayerInterface.h"
#include "Tiled2dMapLayerInterface"
#include "RenderPass.h"
#include "MapInterface.h"

class Tiled2dMapLayer: public Tiled2dMapLayerInterface {
public:

  Tiled2dMapLayer(const std::shared_ptr<MapInterface> &mapInterface);

  virtual std::shared_ptr<LayerInterface> asLayerInterface();

  virtual std::vector<std::shared_ptr<::RenderPassInterface>> buildRenderPasses();

  virtual std::string getIdentifier();

  virtual void pause();

  virtual void resume();

  virtual void hide();

  virtual void show();

private:
  const std::shared_ptr<MapInterface> mapInterface;
};

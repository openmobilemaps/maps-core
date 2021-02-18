#pragma once

#include "GraphicsObjectInterface.h"
#include "RenderPassInterface.h"

class RenderPass : public RenderPassInterface {
  public:
    RenderPass(RenderPassConfig config, std::vector<std::shared_ptr<::GraphicsObjectInterface>> graphicsObjects);
    virtual std::vector<std::shared_ptr<::GraphicsObjectInterface>> getGraphicsObjects() override;
    virtual RenderPassConfig getRenderPassConfig() override;

  private:
    RenderPassConfig config;
    std::vector<std::shared_ptr<::GraphicsObjectInterface>> graphicsObjects;
};

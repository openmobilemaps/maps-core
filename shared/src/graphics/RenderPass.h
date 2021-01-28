#pragma once

#include "RenderPassInterface.h"
#include "GraphicsObjectInterface.h"

class RenderPass: public RenderPassInterface {
public:
    RenderPass(RenderPassConfig config, std::vector<std::shared_ptr<::GraphicsObjectInterface>> graphicsObjects);
    virtual std::vector<std::shared_ptr<::GraphicsObjectInterface>> getGraphicsObjects();
    virtual RenderPassConfig getRenderPassConfig();
private:
    RenderPassConfig config;
    std::vector<std::shared_ptr<::GraphicsObjectInterface>> graphicsObjects;
};

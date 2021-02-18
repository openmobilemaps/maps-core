#include "RenderPass.h"

RenderPass::RenderPass(RenderPassConfig config, std::vector<std::shared_ptr<::GraphicsObjectInterface>> graphicsObjects)
    : config(config)
    , graphicsObjects(graphicsObjects) {}

std::vector<std::shared_ptr<::GraphicsObjectInterface>> RenderPass::getGraphicsObjects() { return graphicsObjects; }

RenderPassConfig RenderPass::getRenderPassConfig() { return config; }

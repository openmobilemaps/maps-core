#include "RenderConfig.h"

RenderConfig::RenderConfig(std::shared_ptr<GraphicsObjectInterface> graphicsObjectInterface, int32_t renderIndex)
    : graphicsObjectInterface(graphicsObjectInterface)
    , renderIndex(renderIndex) {}

std::shared_ptr<::GraphicsObjectInterface> RenderConfig::getGraphicsObject() { return graphicsObjectInterface; }

int32_t RenderConfig::getRenderIndex() { return renderIndex; }

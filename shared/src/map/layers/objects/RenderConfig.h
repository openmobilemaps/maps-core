#pragma once

#include "RenderConfigInterface.h"

class RenderConfig: public RenderConfigInterface {
public:
    virtual ~RenderConfig() {};
    RenderConfig(std::shared_ptr<GraphicsObjectInterface> graphicsObjectInterface, int32_t renderIndex);

    virtual std::shared_ptr<::GraphicsObjectInterface> getGraphicsObject();

    virtual int32_t getRenderIndex();

private:
    int32_t renderIndex;
    std::shared_ptr<GraphicsObjectInterface> graphicsObjectInterface;
};

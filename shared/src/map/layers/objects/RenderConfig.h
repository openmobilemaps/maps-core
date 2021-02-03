#pragma once

#include "RenderConfigInterface.h"

class RenderConfig: public RenderConfigInterface {
public:
    virtual ~RenderConfig() {}

    virtual std::shared_ptr<::GraphicsObjectInterface> getGraphicsObject();

    virtual int32_t getRenderIndex();
};

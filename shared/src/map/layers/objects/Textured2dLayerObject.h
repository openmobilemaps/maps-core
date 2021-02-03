#pragma once
#include "LayerObjectInterface.h"
#include "RenderConfigInterface.h"
#include "Rectangle2dInterface.h"
#include "AlphaShaderInterface.h"
#include "Vec2F.h"


class Textured2dLayerObject: public LayerObjectInterface {
public:
    Textured2dLayerObject(std::shared_ptr<Rectangle2dInterface> rectangle, std::shared_ptr<AlphaShaderInterface> shader);

    virtual ~Textured2dLayerObject() {}

    virtual std::vector<std::shared_ptr<RenderConfigInterface>> getRenderConfig();

private:
    std::shared_ptr<Rectangle2dInterface> rectangle;
    std::shared_ptr<AlphaShaderInterface> shader;
};

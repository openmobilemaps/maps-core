#pragma once
#include "LayerObjectInterface.h"
#include "Polygon2dInterface.h"
#include "ColorShaderInterface.h"
#include "Vec2D.h"
#include "RenderConfig.h"

class Polygon2dLayerObject: public LayerObjectInterface  {
public:
    Polygon2dLayerObject(std::shared_ptr<Polygon2dInterface> polygon, std::shared_ptr<ColorShaderInterface> shader);

    virtual ~Polygon2dLayerObject() {}

    virtual std::vector<std::shared_ptr<RenderConfigInterface>> getRenderConfig();

    void setPositions(std::vector<Vec2D> positions);

    std::shared_ptr<GraphicsObjectInterface> getPolygonObject();

    std::shared_ptr<ShaderProgramInterface> getShaderProgram();

private:
    std::shared_ptr<Polygon2dInterface> polygon;
    std::shared_ptr<ColorShaderInterface> shader;
    std::shared_ptr<RenderConfig> renderConfig;
};

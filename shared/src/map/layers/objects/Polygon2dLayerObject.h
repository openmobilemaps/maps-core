#pragma once

#include "CoordinateConversionHelperInterface.h"
#include "LayerObjectInterface.h"
#include "Polygon2dInterface.h"
#include "ColorShaderInterface.h"
#include "Vec2D.h"
#include "RenderConfig.h"
#include "Coord.h"

class Polygon2dLayerObject : public LayerObjectInterface {
public:
    Polygon2dLayerObject(const std::shared_ptr<CoordinateConversionHelperInterface> &conversionHelper,
                         const std::shared_ptr<Polygon2dInterface> &polygon, const std::shared_ptr<ColorShaderInterface> &shader);

    virtual ~Polygon2dLayerObject() override {}

    virtual std::vector<std::shared_ptr<RenderConfigInterface>> getRenderConfig() override;

    void setPositions(std::vector<Coord> positions);

    std::shared_ptr<GraphicsObjectInterface> getPolygonObject();

    std::shared_ptr<ShaderProgramInterface> getShaderProgram();

private:
    std::shared_ptr<CoordinateConversionHelperInterface> conversionHelper;
    std::shared_ptr<Polygon2dInterface> polygon;
    std::shared_ptr<ColorShaderInterface> shader;
    std::shared_ptr<RenderConfig> renderConfig;
};

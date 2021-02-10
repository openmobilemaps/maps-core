//
// Created by Christoph Maurhofer on 10.02.2021.
//

#pragma once

#include "CoordinateConversionHelperInterface.h"
#include "LayerObjectInterface.h"
#include "Line2dInterface.h"
#include "ColorLineShaderInterface.h"
#include "Vec2D.h"
#include "RenderConfig.h"
#include "Coord.h"

class Line2dLayerObject : public LayerObjectInterface {
public:
    Line2dLayerObject(const std::shared_ptr<CoordinateConversionHelperInterface> &conversionHelper,
                         const std::shared_ptr<Line2dInterface> &line, const std::shared_ptr<ColorLineShaderInterface> &shader);

    ~Line2dLayerObject() {};

    virtual std::vector<std::shared_ptr<RenderConfigInterface>> getRenderConfig();

    void setPositions(std::vector<Coord> positions);

    std::shared_ptr<GraphicsObjectInterface> getLineObject();

    std::shared_ptr<LineShaderProgramInterface> getShaderProgram();

private:
    std::shared_ptr<CoordinateConversionHelperInterface> conversionHelper;
    std::shared_ptr<Line2dInterface> line;
    std::shared_ptr<ColorLineShaderInterface> shader;
    std::vector<std::shared_ptr<RenderConfigInterface>> renderConfig;
};

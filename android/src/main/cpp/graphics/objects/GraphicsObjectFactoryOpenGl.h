//
// Created by Christoph Maurhofer on 28.01.2021.
//

#ifndef MAPSDK_GRAPHICSOBJECTFACTORYOPENGL_H
#define MAPSDK_GRAPHICSOBJECTFACTORYOPENGL_H

#include "GraphicsObjectFactoryInterface.h"

class GraphicsObjectFactoryOpenGl : public GraphicsObjectFactoryInterface {

    virtual std::shared_ptr<Rectangle2dInterface> createRectangle(const std::shared_ptr<::ShaderProgramInterface> &shader) override;

    virtual std::shared_ptr<Line2dInterface> createLine(const std::shared_ptr<::LineShaderProgramInterface> &lineShader) override;

    virtual std::shared_ptr<Polygon2dInterface> createPolygon(const std::shared_ptr<::ShaderProgramInterface> &shader) override;
};

#endif // MAPSDK_GRAPHICSOBJECTFACTORYOPENGL_H

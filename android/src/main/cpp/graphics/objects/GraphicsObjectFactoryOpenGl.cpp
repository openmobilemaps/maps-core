//
// Created by Christoph Maurhofer on 28.01.2021.
//

#include "GraphicsObjectFactoryOpenGl.h"
#include "Line2dOpenGl.h"
#include "Polygon2dOpenGl.h"

std::shared_ptr<Rectangle2dInterface>
GraphicsObjectFactoryOpenGl::createRectangle(const std::shared_ptr<::ShaderProgramInterface> &shader) {
    return std::shared_ptr<Rectangle2dInterface>();
}

std::shared_ptr<Line2dInterface>
GraphicsObjectFactoryOpenGl::createLine(const std::shared_ptr<::LineShaderProgramInterface> &lineShader) {
    return std::make_shared<Line2dOpenGl>(lineShader);
}

std::shared_ptr<Polygon2dInterface>
GraphicsObjectFactoryOpenGl::createPolygon(const std::shared_ptr<::ShaderProgramInterface> &shader) {
    return std::make_shared<Polygon2dOpenGl>(shader);
}

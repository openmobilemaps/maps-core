//
// Created by Christoph Maurhofer on 28.01.2021.
//

#ifndef MAPSDK_LINE2DOPENGL_H
#define MAPSDK_LINE2DOPENGL_H

#include "opengl_wrapper.h"
#include "OpenGlHelper.h"
#include "OpenGlContext.h"
#include "GraphicsObjectInterface.h"
#include "LineShaderProgramInterface.h"
#include "Line2dInterface.h"

class Line2dOpenGl
        : public GraphicsObjectInterface, public Line2dInterface, public std::enable_shared_from_this<GraphicsObjectInterface> {
public:
    Line2dOpenGl(const std::shared_ptr<::LineShaderProgramInterface> &shader);

    virtual ~Line2dOpenGl() {}

    virtual bool isReady();

    virtual void setup(const std::shared_ptr<::RenderingContextInterface> &context);

    virtual void clear();

    virtual void
    render(const std::shared_ptr<::RenderingContextInterface> &context, const ::RenderPassConfig &renderPass, int64_t mvpMatrix);

    virtual void setLinePositions(const std::vector<::Vec2F> &positions);

    virtual std::shared_ptr<GraphicsObjectInterface> asGraphicsObject();

protected:
    void initializeLineAndPoints();

    void drawLineSegments(std::shared_ptr<OpenGlContext> openGlContext, int64_t mvpMatrix);

    void drawPoints(std::shared_ptr<OpenGlContext> openGlContext, int64_t mvpMatrix);

    std::vector<Vec2F> lineCoordinates;

    std::shared_ptr<LineShaderProgramInterface> shaderProgram;
    std::vector<GLfloat> pointsVertexBuffer;
    std::vector<GLfloat> lineVertexBuffer;
    std::vector<GLfloat> lineNormalBuffer;
    std::vector<GLuint> lineIndexBuffer;
    int pointCount;

    bool ready = false;
};


#endif //MAPSDK_LINE2DOPENGL_H

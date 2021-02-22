/*
 * Copyright (c) 2021 Ubique Innovation AG <https://www.ubique.ch>
 *
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this
 *  file, You can obtain one at https://mozilla.org/MPL/2.0/.
 *
 *  SPDX-License-Identifier: MPL-2.0
 */

#ifndef MAPSDK_LINE2DOPENGL_H
#define MAPSDK_LINE2DOPENGL_H

#include "GraphicsObjectInterface.h"
#include "Line2dInterface.h"
#include "LineShaderProgramInterface.h"
#include "OpenGlContext.h"
#include "OpenGlHelper.h"
#include "opengl_wrapper.h"

class Line2dOpenGl : public GraphicsObjectInterface,
                     public Line2dInterface,
                     public std::enable_shared_from_this<GraphicsObjectInterface> {
  public:
    Line2dOpenGl(const std::shared_ptr<::LineShaderProgramInterface> &shader);

    virtual ~Line2dOpenGl() {}

    virtual bool isReady() override;

    virtual void setup(const std::shared_ptr<::RenderingContextInterface> &context) override;

    virtual void clear() override;

    virtual void render(const std::shared_ptr<::RenderingContextInterface> &context, const ::RenderPassConfig &renderPass,
                        int64_t mvpMatrix) override;

    virtual void setLinePositions(const std::vector<::Vec2D> &positions) override;

    virtual std::shared_ptr<GraphicsObjectInterface> asGraphicsObject() override;

  protected:
    void initializeLineAndPoints();

    void drawLineSegments(std::shared_ptr<OpenGlContext> openGlContext, int64_t mvpMatrix);

    void drawPoints(std::shared_ptr<OpenGlContext> openGlContext, int64_t mvpMatrix);

    std::vector<Vec2D> lineCoordinates;

    std::shared_ptr<LineShaderProgramInterface> shaderProgram;
    std::vector<GLfloat> pointsVertexBuffer;
    std::vector<GLfloat> lineVertexBuffer;
    std::vector<GLfloat> lineNormalBuffer;
    std::vector<GLuint> lineIndexBuffer;
    int pointCount;

    bool ready = false;
};

#endif // MAPSDK_LINE2DOPENGL_H

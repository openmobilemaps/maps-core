/*
 * Copyright (c) 2021 Ubique Innovation AG <https://www.ubique.ch>
 *
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this
 *  file, You can obtain one at https://mozilla.org/MPL/2.0/.
 *
 *  SPDX-License-Identifier: MPL-2.0
 */

#pragma once

#include "GraphicsObjectInterface.h"
#include "Line2dInterface.h"
#include "OpenGlContext.h"
#include "OpenGlHelper.h"
#include "ShaderProgramInterface.h"
#include "opengl_wrapper.h"
#include <mutex>

class Line2dOpenGl : public GraphicsObjectInterface,
                     public Line2dInterface,
                     public std::enable_shared_from_this<GraphicsObjectInterface> {
  public:
    Line2dOpenGl(const std::shared_ptr<::ShaderProgramInterface> &shader);

    virtual ~Line2dOpenGl() {}

    virtual bool isReady() override;

    virtual void setup(const std::shared_ptr<::RenderingContextInterface> &context) override;

    virtual void clear() override;

    virtual void render(const std::shared_ptr<::RenderingContextInterface> &context, const ::RenderPassConfig &renderPass,
                        int64_t mvpMatrix, bool isMasked, double screenPixelAsRealMeterFactor) override;

    virtual void setLine(const ::SharedBytes & line, const ::SharedBytes & indices) override;

    virtual std::shared_ptr<GraphicsObjectInterface> asGraphicsObject() override;

    virtual void setIsInverseMasked(bool inversed) override;

  protected:
    void drawLineSegments(std::shared_ptr<OpenGlContext> openGlContext, int64_t mvpMatrix, float widthScaleFactor);

    void prepareGlData(std::shared_ptr<OpenGlContext> openGlContext);

    void removeGlBuffers();

    std::shared_ptr<ShaderProgramInterface> shaderProgram;
    int mvpMatrixHandle;
    int scaleFactorHandle;
    int positionHandle;
    int widthNormalHandle;
    int lengthNormalHandle;
    int pointAHandle;
    int pointBHandle;
    int segmentStartLPosHandle;
    int styleInfoHandle;
    GLuint vertexAttribBuffer;
    std::vector<GLfloat> lineAttributes;
    GLuint indexBuffer;
    std::vector<GLuint> lineIndices;

    bool ready = false;
    bool dataReady = false;
    std::recursive_mutex dataMutex;

    bool isMaskInversed = false;
};

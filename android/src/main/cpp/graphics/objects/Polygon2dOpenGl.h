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
#include "MaskingObjectInterface.h"
#include "OpenGlContext.h"
#include "Polygon2dInterface.h"
#include "ShaderProgramInterface.h"
#include "opengl_wrapper.h"
#include <mutex>

class Polygon2dOpenGl : public GraphicsObjectInterface,
                        public MaskingObjectInterface,
                        public Polygon2dInterface,
                        public std::enable_shared_from_this<Polygon2dOpenGl> {
  public:
    Polygon2dOpenGl(const std::shared_ptr<::ShaderProgramInterface> &shader);

    ~Polygon2dOpenGl(){};

    virtual bool isReady() override;

    virtual void setup(const std::shared_ptr<::RenderingContextInterface> &context) override;

    virtual void clear() override;

    virtual void render(const std::shared_ptr<::RenderingContextInterface> &context, const ::RenderPassConfig &renderPass,
                        int64_t mvpMatrix, bool isMasked, double screenPixelAsRealMeterFactor) override;

    virtual void renderAsMask(const std::shared_ptr<::RenderingContextInterface> &context, const ::RenderPassConfig &renderPass,
                              int64_t mvpMatrix, double screenPixelAsRealMeterFactor) override;

    virtual void setVertices(const ::SharedBytes & vertices, const ::SharedBytes & indices) override;

    virtual std::shared_ptr<GraphicsObjectInterface> asGraphicsObject() override;

    virtual std::shared_ptr<MaskingObjectInterface> asMaskingObject() override;

    virtual void setIsInverseMasked(bool inversed) override;

  protected:
    void prepareGlData(const std::shared_ptr<OpenGlContext> &openGlContext);

    void removeGlBuffers();

    void drawPolygon(std::shared_ptr<OpenGlContext> openGlContext, int program, int64_t mvpMatrix);

    std::shared_ptr<ShaderProgramInterface> shaderProgram;

    int programHandle;
    int mvpMatrixHandle;
    int positionHandle;
    GLuint vertexBuffer;
    std::vector<GLfloat> vertices;
    GLuint indexBuffer;
    std::vector<GLushort> indices;

    bool dataReady = false;
    bool ready = false;
    std::recursive_mutex dataMutex;

    bool isMaskInversed = false;
};

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
#include "OpenGlContext.h"
#include "OpenGlHelper.h"
#include "PolygonGroup2dInterface.h"
#include "RenderLineDescription.h"
#include "ShaderProgramInterface.h"
#include "BaseShaderProgramOpenGl.h"
#include "opengl_wrapper.h"
#include <mutex>

class PolygonGroup2dOpenGl : public GraphicsObjectInterface,
                             public PolygonGroup2dInterface,
                             public std::enable_shared_from_this<GraphicsObjectInterface> {
  public:
    PolygonGroup2dOpenGl(const std::shared_ptr<::BaseShaderProgramOpenGl> &shader);

    virtual ~PolygonGroup2dOpenGl() {}

    // PolygonGroup2dInterface

    virtual void setVertices(const ::SharedBytes & vertices, const ::SharedBytes & indices, const ::Vec3D & origin) override;

    virtual std::shared_ptr<GraphicsObjectInterface> asGraphicsObject() override;

    // GraphicsObjectInterface

    virtual bool isReady() override;

    virtual void setup(const std::shared_ptr<::RenderingContextInterface> &context) override;

    virtual void clear() override;

    virtual void render(const std::shared_ptr<::RenderingContextInterface> &context, const ::RenderPassConfig &renderPass,
                        int64_t vpMatrix, int64_t mMatrix, const ::Vec3D & origin, bool isMasked, double screenPixelAsRealMeterFactor) override;

    virtual void setIsInverseMasked(bool inversed) override;

    void setDebugLabel(const std::string &label) override;

protected:

    virtual void removeGlBuffers();

    std::shared_ptr<BaseShaderProgramOpenGl> shaderProgram;
    std::string programName;
    int program = 0;

    int vpMatrixHandle;
    int mMatrixHandle;
    int originOffsetHandle;
    int scaleFactorHandle;
    int positionHandle;
    int styleIndexHandle;
    GLuint vao;
    GLuint attribBuffer;
    std::vector<GLfloat> polygonAttributes;
    GLuint indexBuffer;
    std::vector<GLushort> polygonIndices;
    bool glDataBuffersGenerated = false;
    Vec3D polygonOrigin = Vec3D(0.0, 0.0, 0.0);

    bool dataReady = false;
    bool ready = false;
    std::recursive_mutex dataMutex;

    bool isMaskInversed = false;
};

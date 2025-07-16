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
#include "LineGroup2dInterface.h"
#include "OpenGlContext.h"
#include "OpenGlHelper.h"
#include "RenderLineDescription.h"
#include "ShaderProgramInterface.h"
#include "BaseShaderProgramOpenGl.h"
#include "opengl_wrapper.h"
#include <mutex>

class LineGroup2dOpenGl : public GraphicsObjectInterface,
                          public LineGroup2dInterface,
                          public std::enable_shared_from_this<GraphicsObjectInterface> {
  public:
    LineGroup2dOpenGl(const std::shared_ptr<BaseShaderProgramOpenGl> &shader);

    virtual ~LineGroup2dOpenGl() {}

    // LineGroup2dInterface

    virtual void setLines(const ::SharedBytes & lines, const ::SharedBytes & indices, const Vec3D &origin, bool is3d) override;

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

    bool is3d = false;
    std::shared_ptr<BaseShaderProgramOpenGl> shaderProgram;
    std::string programName;
    int program = 0;

    int vpMatrixHandle;
    int mMatrixHandle;
    int originOffsetHandle;
    int lineOriginHandle;
    int scaleFactorHandle;

    int positionHandle;
    int extrudeHandle;
    int lineSideHandle;
    int lengthPrefixHandle;
    int lengthCorrectionHandle;
    int stylingIndexHandle;

    GLuint vao;
    GLuint vertexAttribBuffer = -1;
    std::vector<GLfloat> lineAttributes;
    GLuint indexBuffer = -1;
    std::vector<GLuint> lineIndices;
    bool glDataBuffersGenerated = false;
    Vec3D lineOrigin = Vec3D(0.0, 0.0, 0.0);

    bool ready = false;
    bool dataReady = false;
    std::recursive_mutex dataMutex;

    bool isMaskInversed = false;
};

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
#include "BaseShaderProgramOpenGl.h"
#include "opengl_wrapper.h"
#include <mutex>

class Polygon2dTessellatedOpenGl : public GraphicsObjectInterface,
                                   public MaskingObjectInterface,
                                   public Polygon2dInterface,
                                   public std::enable_shared_from_this<Polygon2dTessellatedOpenGl> {
  public:
    Polygon2dTessellatedOpenGl(const std::shared_ptr<::BaseShaderProgramOpenGl> &shader);

    ~Polygon2dTessellatedOpenGl(){};

    virtual bool isReady() override;

    virtual void setup(const std::shared_ptr<::RenderingContextInterface> &context) override;

    virtual void clear() override;

    virtual void render(const std::shared_ptr<::RenderingContextInterface> &context, const ::RenderPassConfig &renderPass,
                        int64_t vpMatrix, int64_t mMatrix, const ::Vec3D & origin, bool isMasked,
                        double screenPixelAsRealMeterFactor, bool isScreenSpaceCoords) override;

    virtual void renderAsMask(const std::shared_ptr<::RenderingContextInterface> &context, const ::RenderPassConfig &renderPass,
                              int64_t vpMatrix, int64_t mMatrix, const ::Vec3D & origin,
                              double screenPixelAsRealMeterFactor, bool isScreenSpaceCoords) override;

    void setSubdivisionFactor(int32_t factor) override;

    virtual void setVertices(const ::SharedBytes & vertices, const ::SharedBytes & indices, const ::Vec3D & origin, bool is3d) override;

    virtual std::shared_ptr<GraphicsObjectInterface> asGraphicsObject() override;

    virtual std::shared_ptr<MaskingObjectInterface> asMaskingObject() override;

    virtual void setIsInverseMasked(bool inversed) override;

    void setDebugLabel(const std::string &label) override;

protected:
    void prepareGlData(int program);

    void removeGlBuffers();

    inline void drawPolygon(const std::shared_ptr<::RenderingContextInterface> &context, int program, int64_t vpMatrix,
                            int64_t mMatrix, const Vec3D &origin, bool isScreenSpaceCoords);

    std::shared_ptr<BaseShaderProgramOpenGl> shaderProgram;
    std::string programName;
    int program = 0;

    int mMatrixHandle{};
    int originOffsetHandle{};
    int subdivisionFactorHandle{};
    int originHandle{};
    int is3dHandle{};
    int positionHandle{};
    int frameCoordHandle{};
    GLuint vao{};
    GLuint vertexBuffer{};
    std::vector<GLfloat> vertices;
    GLuint indexBuffer{};
    std::vector<GLushort> indices;
    bool glDataBuffersGenerated = false;
    Vec3D polygonOrigin = Vec3D(0.0, 0.0, 0.0);

    bool is3d = false;
    int32_t subdivisionFactor = 0;

    bool dataReady = false;
    bool ready = false;
    std::recursive_mutex dataMutex;

    bool isMaskInversed = false;
};

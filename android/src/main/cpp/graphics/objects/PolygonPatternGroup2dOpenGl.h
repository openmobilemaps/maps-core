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
#include "PolygonPatternGroup2dInterface.h"
#include "ShaderProgramInterface.h"
#include "BaseShaderProgramOpenGl.h"
#include "opengl_wrapper.h"
#include <mutex>
#include <vector>
#include <RectD.h>
#include <Quad2dD.h>

class PolygonPatternGroup2dOpenGl : public GraphicsObjectInterface,
                     public MaskingObjectInterface,
                     public PolygonPatternGroup2dInterface,
                     public std::enable_shared_from_this<PolygonPatternGroup2dOpenGl> {
  public:
    PolygonPatternGroup2dOpenGl(const std::shared_ptr<::BaseShaderProgramOpenGl> &shader);

    ~PolygonPatternGroup2dOpenGl(){};

    virtual bool isReady() override;

    virtual void setup(const std::shared_ptr<::RenderingContextInterface> &context) override;

    virtual void clear() override;

    virtual void renderAsMask(const std::shared_ptr<::RenderingContextInterface> &context, const ::RenderPassConfig &renderPass,
                              int64_t vpMatrix, int64_t mMatrix, const ::Vec3D & origin, double screenPixelAsRealMeterFactor) override;

    virtual void render(const std::shared_ptr<::RenderingContextInterface> &context, const ::RenderPassConfig &renderPass,
                        int64_t vpMatrix, int64_t mMatrix, const ::Vec3D & origin, bool isMasked, double screenPixelAsRealMeterFactor) override;

    virtual void loadTexture(const std::shared_ptr<::RenderingContextInterface> &context,
                             const std::shared_ptr<TextureHolderInterface> &textureHolder) override;

    virtual void removeTexture() override;

    virtual std::shared_ptr<GraphicsObjectInterface> asGraphicsObject() override;

    virtual void setIsInverseMasked(bool inversed) override;

    void setDebugLabel(const std::string &label) override;

    void setVertices(const SharedBytes &vertices, const SharedBytes &indices, const ::Vec3D & origin) override;

    void setTextureCoordinates(const ::SharedBytes &textureCoordinates) override;

    void setOpacities(const SharedBytes &values) override;

    void setScalingFactor(float factor) override;

    void setScalingFactors(const ::Vec2F & factor) override;

protected:
    virtual void prepareTextureDraw(int mProgram);

    void prepareGlData(int program);

    void removeGlBuffers();

    std::shared_ptr<BaseShaderProgramOpenGl> shaderProgram;
    std::string programName;
    int program;

    int mMatrixHandle;
    int originOffsetHandle;
    int positionHandle;
    int styleIndexHandle;
    GLuint vao;
    GLuint vertexBuffer;
    std::vector<GLfloat> vertices;
    GLuint indexBuffer;
    std::vector<GLushort> indices;
    bool glDataBuffersGenerated = false;
    Vec3D polygonOrigin = Vec3D(0.0, 0.0, 0.0);

    std::shared_ptr<TextureHolderInterface> textureHolder;
    int texturePointer;

    double factorHeight = 1.0;
    double factorWidth = 1.0;

    bool ready = false;
    bool dataReady = false;
    uint8_t buffersNotReady = 0b00000011;
    std::recursive_mutex dataMutex;

    bool isMaskInversed = false;

    ::Vec2F scalingFactor = Vec2F(1.0, 1.0);
    std::vector<GLfloat> opacities;
    std::vector<GLfloat> textureCoordinates;
    GLuint textureCoordinatesBuffer;

    const int maxNumStyles = 16;
    const int sizeOpacitiesValuesArray = maxNumStyles;
    const int sizeTextureCoordinatesValuesArray = 5 * 16;

};

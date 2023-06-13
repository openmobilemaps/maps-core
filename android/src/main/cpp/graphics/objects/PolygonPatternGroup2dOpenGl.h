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
    PolygonPatternGroup2dOpenGl(const std::shared_ptr<::ShaderProgramInterface> &shader);

    ~PolygonPatternGroup2dOpenGl(){};

    virtual bool isReady() override;

    virtual void setup(const std::shared_ptr<::RenderingContextInterface> &context) override;

    virtual void clear() override;

    virtual void renderAsMask(const std::shared_ptr<::RenderingContextInterface> &context, const ::RenderPassConfig &renderPass,
                              int64_t mvpMatrix, double screenPixelAsRealMeterFactor) override;

    virtual void render(const std::shared_ptr<::RenderingContextInterface> &context, const ::RenderPassConfig &renderPass,
                        int64_t mvpMatrix, bool isMasked, double screenPixelAsRealMeterFactor) override;

    virtual void loadTexture(const std::shared_ptr<::RenderingContextInterface> &context,
                             const std::shared_ptr<TextureHolderInterface> &textureHolder) override;

    virtual void removeTexture() override;

    virtual std::shared_ptr<GraphicsObjectInterface> asGraphicsObject() override;

    virtual void setIsInverseMasked(bool inversed) override;

    void setVertices(const SharedBytes &vertices, const SharedBytes &indices) override;

    void setTextureCoordinates(const ::SharedBytes &textureCoordinates) override;

    void setOpacities(const SharedBytes &values) override;

    void setScalingFactor(float factor) override;

protected:
    virtual void prepareTextureDraw(int mProgram);

    void prepareGlData(const int &programHandle);

    void removeGlBuffers();

    std::shared_ptr<ShaderProgramInterface> shaderProgram;

    int programHandle;
    int mvpMatrixHandle;
    int positionHandle;
    int styleIndexHandle;
    GLuint vertexBuffer;
    std::vector<GLfloat> vertices;
    GLuint indexBuffer;
    std::vector<GLushort> indices;

    std::shared_ptr<TextureHolderInterface> textureHolder;
    int texturePointer;

    double factorHeight = 1.0;
    double factorWidth = 1.0;

    bool ready = false;
    bool dataReady = false;
    uint8_t buffersNotReady = 0b00000011;
    std::recursive_mutex dataMutex;

    bool isMaskInversed = false;

    float scalingFactor = 1.0;
    std::vector<GLfloat> opacities;
    std::vector<GLfloat> textureCoordinates;
    GLuint textureCoordinatesBuffer;

    const int maxNumStyles = 16;
    const int sizeOpacitiesValuesArray = maxNumStyles;
    const int sizeTextureCoordinatesValuesArray = 5 * 16;

};

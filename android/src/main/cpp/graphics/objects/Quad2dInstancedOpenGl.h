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
#include "Quad2dInstancedInterface.h"
#include "ShaderProgramInterface.h"
#include "opengl_wrapper.h"
#include <mutex>
#include <vector>
#include <RectD.h>

class Quad2dInstancedOpenGl : public GraphicsObjectInterface,
                     public MaskingObjectInterface,
                     public Quad2dInstancedInterface,
                     public std::enable_shared_from_this<Quad2dInstancedOpenGl> {
  public:
    Quad2dInstancedOpenGl(const std::shared_ptr<::ShaderProgramInterface> &shader);

    ~Quad2dInstancedOpenGl(){};

    virtual bool isReady() override;

    virtual void setup(const std::shared_ptr<::RenderingContextInterface> &context) override;

    virtual void clear() override;

    virtual void renderAsMask(const std::shared_ptr<::RenderingContextInterface> &context, const ::RenderPassConfig &renderPass,
                              int64_t vpMatrix, int64_t mMatrix, double screenPixelAsRealMeterFactor) override;

    virtual void render(const std::shared_ptr<::RenderingContextInterface> &context, const ::RenderPassConfig &renderPass,
                        int64_t vpMatrix, int64_t mMatrix, bool isMasked, double screenPixelAsRealMeterFactor) override;

    virtual void setFrame(const ::Quad2dD &frame) override;

    virtual void loadTexture(const std::shared_ptr<::RenderingContextInterface> &context,
                             const std::shared_ptr<TextureHolderInterface> &textureHolder) override;

    virtual void removeTexture() override;

    virtual std::shared_ptr<GraphicsObjectInterface> asGraphicsObject() override;

    virtual std::shared_ptr<MaskingObjectInterface> asMaskingObject() override;

    virtual void setIsInverseMasked(bool inversed) override;

    void setDebugLabel(const std::string &label) override;

    virtual void setInstanceCount(int32_t count) override;

    virtual void setPositions(const ::SharedBytes &positions) override;

    virtual void setRotations(const ::SharedBytes &rotations) override;

    virtual void setScales(const ::SharedBytes &scales) override;

    virtual void setTextureCoordinates(const ::SharedBytes &textureCoordinates) override;

    virtual void setAlphas(const ::SharedBytes &values) override;

  protected:
    virtual void adjustTextureCoordinates();

    virtual void prepareTextureDraw(int mProgram);

    void prepareGlData(int program);

    void prepareTextureCoordsGlData(int program);

    void removeGlBuffers();

    void removeTextureCoordsGlBuffers();

    std::shared_ptr<ShaderProgramInterface> shaderProgram;
    std::string programName;
    int program;

    int vpMatrixHandle;
    int mMatrixHandle;
    int positionHandle;
    GLuint vertexBuffer;
    std::vector<GLfloat> vertices;
    int textureCoordinateHandle;
    GLuint textureCoordsBuffer;
    std::vector<GLfloat> textureCoords;
    GLuint indexBuffer;
    std::vector<GLubyte> indices;
    bool glDataBuffersGenerated = false;

    std::shared_ptr<TextureHolderInterface> textureHolder;
    int texturePointer;

    bool usesTextureCoords = false;

    Quad2dD frame = Quad2dD(Vec2D(0.0, 0.0), Vec2D(0.0, 0.0), Vec2D(0.0, 0.0), Vec2D(0.0, 0.0));
    RectD textureCoordinates = RectD(0.0, 0.0, 1.0, 1.0);
    double factorHeight = 1.0;
    double factorWidth = 1.0;

    bool ready = false;
    uint8_t buffersNotReady = 0b00011111;
    bool textureCoordsReady = false;
    std::recursive_mutex dataMutex;

    bool isMaskInversed = false;

    int instanceCount = 0;

    GLuint dynamicInstanceDataBuffer;
    int instPositionsHandle;
    int instRotationsHandle;
    int instScalesHandle;
    int instAlphasHandle;
    int instTextureCoordinatesHandle;

    static const int instPositionsOffsetBytes = sizeof(GLfloat) * 0;
    static const int instRotationsOffsetBytes = sizeof(GLfloat) * 2;
    static const int instTextureCoordinatesOffsetBytes = sizeof(GLfloat) * 3;
    static const int instScalesOffsetBytes = sizeof(GLfloat) * 7;
    static const int instAlphasOffsetBytes = sizeof(GLfloat) * 9;
    static const int instValuesSizeBytes = sizeof(GLfloat) * 10;

private:
    bool writeToDynamicInstanceDataBuffer(const ::SharedBytes &data, int targetOffsetBytes);

};

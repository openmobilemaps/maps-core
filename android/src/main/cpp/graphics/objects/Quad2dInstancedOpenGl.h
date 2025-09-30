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
#include "BaseShaderProgramOpenGl.h"
#include "opengl_wrapper.h"
#include <mutex>
#include <vector>
#include <RectD.h>

class Quad2dInstancedOpenGl : public GraphicsObjectInterface,
                     public MaskingObjectInterface,
                     public Quad2dInstancedInterface,
                     public std::enable_shared_from_this<Quad2dInstancedOpenGl> {
  public:
    Quad2dInstancedOpenGl(const std::shared_ptr<::BaseShaderProgramOpenGl> &shader);

    ~Quad2dInstancedOpenGl(){};

    virtual bool isReady() override;

    virtual void setup(const std::shared_ptr<::RenderingContextInterface> &context) override;

    virtual void clear() override;

    virtual void renderAsMask(const std::shared_ptr<::RenderingContextInterface> &context, const ::RenderPassConfig &renderPass,
                              int64_t vpMatrix, int64_t mMatrix, const ::Vec3D & origin, double screenPixelAsRealMeterFactor) override;

    virtual void render(const std::shared_ptr<::RenderingContextInterface> &context, const ::RenderPassConfig &renderPass,
                        int64_t vpMatrix, int64_t mMatrix, const ::Vec3D & origin, bool isMasked, double screenPixelAsRealMeterFactor) override;

    virtual void setFrame(const ::Quad2dD &frame, const Vec3D &origin, bool is3d) override;

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

    void setPositionOffset(const SharedBytes &offsets) override;

protected:
    virtual void adjustTextureCoordinates();

    virtual void prepareTextureDraw(int mProgram);

    void prepareGlData(int program);

    void prepareTextureCoordsGlData(int program);

    void removeGlBuffers();

    void removeTextureCoordsGlBuffers();

    bool is3d = false;
    std::shared_ptr<BaseShaderProgramOpenGl> shaderProgram;
    std::string programName;
    int program;

    int mMatrixHandle;
    int originOffsetHandle;
    int positionHandle;
    GLuint vao;
    GLuint vertexBuffer;
    std::vector<GLfloat> vertices;
    int textureCoordinateHandle;
    GLuint textureCoordsBuffer;
    std::vector<GLfloat> textureCoords;
    GLuint indexBuffer;
    std::vector<GLubyte> indices;
    bool glDataBuffersGenerated = false;
    bool texCoordBufferGenerated = false;
    Vec3D quadsOrigin = Vec3D(0.0, 0.0, 0.0);

    std::shared_ptr<TextureHolderInterface> textureHolder;
    int texturePointer;

    bool usesTextureCoords = false;

    Quad2dD frame = Quad2dD(Vec2D(0.0, 0.0), Vec2D(0.0, 0.0), Vec2D(0.0, 0.0), Vec2D(0.0, 0.0));
    RectD textureCoordinates = RectD(0.0, 0.0, 1.0, 1.0);
    double factorHeight = 1.0;
    double factorWidth = 1.0;

    bool ready = false;
    uint8_t buffersNotReadyResetValue = 0b00111111;
    uint8_t buffersNotReady = 0b00111111;
    bool textureCoordsReady = false;
    std::recursive_mutex dataMutex;

    bool isMaskInversed = false;

    int instanceCount = 0;
    int bufferInstanceCapacity = 0;

    GLuint dynamicInstanceDataBuffer;
    int instPositionsHandle;
    int instRotationsHandle;
    int instTextureCoordinatesHandle;
    int instScalesHandle;
    int instAlphasHandle;
    int instPositionOffsetsHandle;

    static const uintptr_t instPositionsOffsetBytes = sizeof(GLfloat) * 0;
    static const uintptr_t instRotationsOffsetBytes = sizeof(GLfloat) * 2;
    static const uintptr_t instTextureCoordinatesOffsetBytes = sizeof(GLfloat) * 3;
    static const uintptr_t instScalesOffsetBytes = sizeof(GLfloat) * 7;
    static const uintptr_t instAlphasOffsetBytes = sizeof(GLfloat) * 9;
    static const uintptr_t instPositionOffsetsOffsetBytes = sizeof(GLfloat) * 10;
    static const uintptr_t instValuesSizeBytes = sizeof(GLfloat) * 12;

    static const uintptr_t instPositionsOffsetBytes3d = sizeof(GLfloat) * 0;
    static const uintptr_t instRotationsOffsetBytes3d = sizeof(GLfloat) * 3;
    static const uintptr_t instTextureCoordinatesOffsetBytes3d = sizeof(GLfloat) * 4;
    static const uintptr_t instScalesOffsetBytes3d = sizeof(GLfloat) * 8;
    static const uintptr_t instAlphasOffsetBytes3d = sizeof(GLfloat) * 10;
    static const uintptr_t instPositionOffsetsOffsetBytes3d = sizeof(GLfloat) * 11;
    static const uintptr_t instValuesSizeBytes3d = sizeof(GLfloat) * 13;

private:
    bool writeToDynamicInstanceDataBuffer(const ::SharedBytes &data, int targetOffsetBytes);
    void setBufferInstanceCapacity(int count);
};

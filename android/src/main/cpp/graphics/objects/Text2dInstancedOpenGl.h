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
#include "TextInstancedInterface.h"
#include "ShaderProgramInterface.h"
#include "BaseShaderProgramOpenGl.h"
#include "opengl_wrapper.h"
#include <mutex>
#include <vector>
#include <RectD.h>

class Text2dInstancedOpenGl : public GraphicsObjectInterface,
                              public TextInstancedInterface,
                              public std::enable_shared_from_this<Text2dInstancedOpenGl> {
public:
    Text2dInstancedOpenGl(const std::shared_ptr<::BaseShaderProgramOpenGl> &shader);

    ~Text2dInstancedOpenGl(){};

    virtual bool isReady() override;

    virtual void setup(const std::shared_ptr<::RenderingContextInterface> &context) override;

    virtual void clear() override;

    virtual void render(const std::shared_ptr<::RenderingContextInterface> &context, const ::RenderPassConfig &renderPass,
                        int64_t vpMatrix, int64_t mMatrix, const ::Vec3D & origin, bool isMasked,
                        double screenPixelAsRealMeterFactor, bool isScreenSpaceCoords) override;

    virtual void setFrame(const ::Quad2dD &frame, const Vec3D &origin, bool is3d) override;

    virtual void loadFont(const std::shared_ptr<::RenderingContextInterface> &context, const ::FontData &fontData,
                          const /*not-null*/ std::shared_ptr<TextureHolderInterface> &fontMsdfTexture) override;

    virtual void removeTexture() override;

    virtual std::shared_ptr<GraphicsObjectInterface> asGraphicsObject() override;

    virtual void setIsInverseMasked(bool inversed) override;

    void setDebugLabel(const std::string &label) override;

    virtual void setInstanceCount(int32_t count) override;

    virtual void setPositions(const ::SharedBytes &positions) override;

    virtual void setRotations(const ::SharedBytes &rotations) override;

    virtual void setScales(const ::SharedBytes &scales) override;

    virtual void setAlphas(const ::SharedBytes &alphas) override;

    void setTextureCoordinates(const SharedBytes &textureCoordinates) override;

    void setReferencePositions(const SharedBytes &positions) override;

    virtual void setStyleIndices(const ::SharedBytes &indices) override;

    virtual void setStyles(const ::SharedBytes &values) override;

protected:
    virtual void adjustTextureCoordinates();

    virtual void prepareTextureDraw(int program);

    void prepareGlData(int program);

    void prepareTextureCoordsGlData(int program);

    void removeGlBuffers();

    void removeTextureCoordsGlBuffers();

    bool is3d = false;
    std::shared_ptr<BaseShaderProgramOpenGl> shaderProgram;
    std::string programName;
    int program;

    int vpMatrixHandle;
    int mMatrixHandle;
    int originOffsetHandle;
    int originHandle;
    int positionHandle;
    int isHaloHandle;
    int textureFactorHandle;
    int distanceRangeHandle;
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

    // Font texture
    std::shared_ptr<TextureHolderInterface> textureHolder;
    int texturePointer;
    // Font metadata
    float distanceRange;

    bool usesTextureCoords = false;

    Quad2dD frame = Quad2dD(Vec2D(0.0, 0.0), Vec2D(0.0, 0.0), Vec2D(0.0, 0.0), Vec2D(0.0, 0.0));
    RectD textureCoordinates = RectD(0.0, 0.0, 1.0, 1.0);
    double factorHeight = 1.0;
    double factorWidth = 1.0;

    bool ready = false;
    uint8_t buffersNotReadyResetValue = 0b11111111;
    uint8_t buffersNotReady = buffersNotReadyResetValue;
    bool textureCoordsReady = false;
    std::recursive_mutex dataMutex;

    bool isMaskInversed = false;

    int instanceCount = 0;

    GLuint dynamicInstanceDataBuffer;
    int instPositionsHandle;
    int instRotationsHandle;
    int instAlphasHandle;
    int instScalesHandle;
    int instStyleIndicesHandle;
    int instTextureCoordinatesHandle;
    int instReferencePositionsHandle;

    GLuint textStyleBuffer;

    static const uintptr_t instPositionsOffsetBytes = sizeof(GLfloat) * 0;
    static const uintptr_t instTextureCoordinatesOffsetBytes = sizeof(GLfloat) * 2;
    static const uintptr_t instScalesOffsetBytes = sizeof(GLfloat) * 6;
    static const uintptr_t instAlphasOffsetBytes = sizeof(GLfloat) * 8;
    static const uintptr_t instRotationsOffsetBytes = sizeof(GLfloat) * 9;
    static const uintptr_t instStyleIndicesOffsetBytes = sizeof(GLfloat) * 10;
    static const uintptr_t instReferencePositionsOffsetBytes = sizeof(GLfloat) * 11;
    static const uintptr_t instValuesSizeBytes = sizeof(GLfloat) * 13;

    static const uintptr_t instPositionsOffsetBytes3d = sizeof(GLfloat) * 0;
    static const uintptr_t instTextureCoordinatesOffsetBytes3d = sizeof(GLfloat) * 2;
    static const uintptr_t instScalesOffsetBytes3d = sizeof(GLfloat) * 6;
    static const uintptr_t instAlphasOffsetBytes3d = sizeof(GLfloat) * 8;
    static const uintptr_t instRotationsOffsetBytes3d = sizeof(GLfloat) * 9;
    static const uintptr_t instStyleIndicesOffsetBytes3d = sizeof(GLfloat) * 10;
    static const uintptr_t instReferencePositionsOffsetBytes3d = sizeof(GLfloat) * 11;
    static const uintptr_t instValuesSizeBytes3d = sizeof(GLfloat) * 14;

private:
    bool writeToDynamicInstanceDataBuffer(const ::SharedBytes &data, GLuint targetOffsetBytes);

    static const GLuint STYLE_UBO_BINDING = 1;
};

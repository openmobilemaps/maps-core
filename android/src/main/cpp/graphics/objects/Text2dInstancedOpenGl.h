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
#include "opengl_wrapper.h"
#include <mutex>
#include <vector>
#include <RectD.h>

class Text2dInstancedOpenGl : public GraphicsObjectInterface,
                              public TextInstancedInterface,
                              public std::enable_shared_from_this<Text2dInstancedOpenGl> {
public:
    Text2dInstancedOpenGl(const std::shared_ptr<::ShaderProgramInterface> &shader);

    ~Text2dInstancedOpenGl(){};

    virtual bool isReady() override;

    virtual void setup(const std::shared_ptr<::RenderingContextInterface> &context) override;

    virtual void clear() override;

    virtual void render(const std::shared_ptr<::RenderingContextInterface> &context, const ::RenderPassConfig &renderPass,
                        int64_t mvpMatrix, bool isMasked, double screenPixelAsRealMeterFactor) override;

    virtual void setFrame(const ::Quad2dD &frame) override;

    virtual void loadTexture(const std::shared_ptr<::RenderingContextInterface> &context,
                             const std::shared_ptr<TextureHolderInterface> &textureHolder) override;

    virtual void removeTexture() override;

    virtual std::shared_ptr<GraphicsObjectInterface> asGraphicsObject() override;

    virtual void setIsInverseMasked(bool inversed) override;

    virtual void setInstanceCount(int32_t count) override;

    virtual void setPositions(const ::SharedBytes &positions) override;

    virtual void setRotations(const ::SharedBytes &rotations) override;

    virtual void setScales(const ::SharedBytes &scales) override;

    void setTextureCoordinates(const SharedBytes &textureCoordinates) override;

    virtual void setStyleIndices(const ::SharedBytes &indices) override;

    virtual void setStyles(const ::SharedBytes &values) override;

protected:
    virtual void adjustTextureCoordinates();

    virtual void prepareTextureDraw(std::shared_ptr<OpenGlContext> &openGLContext, int mProgram);

    void prepareGlData(const std::shared_ptr<OpenGlContext> &openGlContext, const int &programHandle);

    void prepareTextureCoordsGlData(const std::shared_ptr<OpenGlContext> &openGlContext, const int &programHandle);

    void removeGlBuffers();

    void removeTextureCoordsGlBuffers();

    std::shared_ptr<ShaderProgramInterface> shaderProgram;

    int programHandle;
    int mvpMatrixHandle;
    int positionHandle;
    GLuint vertexBuffer;
    std::vector<GLfloat> vertices;
    int textureCoordinateHandle;
    GLuint textureCoordsBuffer;
    std::vector<GLfloat> textureCoords;
    GLuint indexBuffer;
    std::vector<GLubyte> indices;

    std::shared_ptr<TextureHolderInterface> textureHolder;
    int texturePointer;

    bool usesTextureCoords = false;

    Quad2dD frame = Quad2dD(Vec2D(0.0, 0.0), Vec2D(0.0, 0.0), Vec2D(0.0, 0.0), Vec2D(0.0, 0.0));
    RectD textureCoordinates = RectD(0.0, 0.0, 1.0, 1.0);
    double factorHeight = 1.0;
    double factorWidth = 1.0;

    bool ready = false;
    uint8_t buffersNotReady = 0b00111111;
    bool textureCoordsReady = false;
    std::recursive_mutex dataMutex;

    bool isMaskInversed = false;

    int instanceCount = 0;

    int instPositionsHandle;
    GLuint positionsBuffer;
    int instRotationsHandle;
    GLuint rotationsBuffer;
    int instScalesHandle;
    GLuint scalesBuffer;
    int instStyleIndicesHandle;
    GLuint styleIndicesBuffer;
    int styleBufferHandle;
    GLuint styleBuffer;
    int instTextureCoordinatesHandle;
    GLuint textureCoordinatesListBuffer;

private:
    bool writeToBuffer(const ::SharedBytes &data, GLuint target);

};

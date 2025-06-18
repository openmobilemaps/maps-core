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

#include "GlyphDescription.h"
#include "GraphicsObjectInterface.h"
#include "MaskingObjectInterface.h"
#include "OpenGlContext.h"
#include "ShaderProgramInterface.h"
#include "BaseShaderProgramOpenGl.h"
#include "TextDescription.h"
#include "TextInterface.h"
#include "opengl_wrapper.h"
#include <mutex>
#include <vector>

class Text2dOpenGl : public GraphicsObjectInterface,
                     public MaskingObjectInterface,
                     public TextInterface,
                     public std::enable_shared_from_this<Text2dOpenGl> {
  public:
    Text2dOpenGl(const std::shared_ptr<::BaseShaderProgramOpenGl> &shader);

    ~Text2dOpenGl(){};

    virtual bool isReady() override;

    virtual void setup(const std::shared_ptr<::RenderingContextInterface> &context) override;

    virtual void clear() override;

    virtual void renderAsMask(const std::shared_ptr<::RenderingContextInterface> &context, const ::RenderPassConfig &renderPass,
                              int64_t vpMatrix, int64_t mMatrix, const ::Vec3D & origin, double screenPixelAsRealMeterFactor) override;

    virtual void render(const std::shared_ptr<::RenderingContextInterface> &context, const ::RenderPassConfig &renderPass,
                        int64_t vpMatrix, int64_t mMatrix, const ::Vec3D & origin, bool isMasked, double screenPixelAsRealMeterFactor) override;

    void setTextsShared(const SharedBytes &vertices, const SharedBytes &indices) override;

    virtual void setTextsLegacy(const std::vector<TextDescription> &texts);

    virtual void loadTexture(const std::shared_ptr<::RenderingContextInterface> &context,
                             const std::shared_ptr<TextureHolderInterface> &textureHolder) override;
    virtual void removeTexture() override;

    virtual std::shared_ptr<GraphicsObjectInterface> asGraphicsObject() override;

    virtual void setIsInverseMasked(bool inversed) override;

    void setDebugLabel(const std::string &label) override;

protected:
    virtual void prepareTextureDraw(int program);

    void prepareGlData(int program);

    void removeGlBuffers();

    std::shared_ptr<BaseShaderProgramOpenGl> shaderProgram;
    std::string programName;
    int program;

    int vpMatrixHandle = -1;
    int mMatrixHandle = -1;
    int positionHandle = -1;
    int textureCoordinateHandle = -1;
    GLuint vao;
    GLuint vertexAttribBuffer;
    std::vector<GLfloat> textVertexAttributes;
    GLuint indexBuffer;
    std::vector<GLushort> textIndices;
    bool glDataBuffersGenerated = false;

    std::shared_ptr<TextureHolderInterface> textureHolder;
    int texturePointer;
    int textureCoordScaleFactorHandle = -1;
    std::vector<GLfloat> textureCoordScaleFactor = {1.0, 1.0};

    bool ready = false;
    bool textureCoordsReady = false;
    bool dataReady = false;
    std::recursive_mutex dataMutex;

    bool isMaskInversed = false;
};

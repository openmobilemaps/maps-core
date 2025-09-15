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
#include "Quad2dInterface.h"
#include "ShaderProgramInterface.h"
#include "BaseShaderProgramOpenGl.h"
#include "opengl_wrapper.h"
#include <mutex>
#include <vector>

class Quad2dOpenGl : public GraphicsObjectInterface,
                     public MaskingObjectInterface,
                     public Quad2dInterface,
                     public std::enable_shared_from_this<Quad2dOpenGl> {
  public:
    Quad2dOpenGl(const std::shared_ptr<::BaseShaderProgramOpenGl> &shader);

    ~Quad2dOpenGl(){};

    virtual bool isReady() override;

    virtual void setup(const std::shared_ptr<::RenderingContextInterface> &context) override;

    virtual void clear() override;

    virtual void renderAsMask(const std::shared_ptr<::RenderingContextInterface> &context, const ::RenderPassConfig &renderPass,
                              int64_t vpMatrix, int64_t mMatrix, const ::Vec3D & origin, double screenPixelAsRealMeterFactor,
                              bool isScreenSpaceCoords) override;

    virtual void render(const std::shared_ptr<::RenderingContextInterface> &context, const ::RenderPassConfig &renderPass,
                        int64_t vpMatrix, int64_t mMatrix, const ::Vec3D & origin, bool isMasked,
                        double screenPixelAsRealMeterFactor, bool isScreenSpaceCoords) override;

    virtual void setFrame(const ::Quad3dD &frame, const ::RectD &textureCoordinates, const Vec3D &origin, bool is3D) override;

    void setSubdivisionFactor(int32_t factor) override;

    void setMinMagFilter(TextureFilterType filterType) override;

    virtual void loadTexture(const std::shared_ptr<::RenderingContextInterface> &context,
                             const std::shared_ptr<TextureHolderInterface> &textureHolder) override;

    virtual void removeTexture() override;

    virtual std::shared_ptr<GraphicsObjectInterface> asGraphicsObject() override;

    virtual std::shared_ptr<MaskingObjectInterface> asMaskingObject() override;

    virtual void setIsInverseMasked(bool inversed) override;

    void setDebugLabel(const std::string &label) override;

protected:
    void computeGeometry(bool texCoordsOnly);

    void prepareGlData(int program);

    void prepareTextureCoordsGlData(int program);

    void removeGlBuffers();

    void removeTextureCoordsGlBuffers();

    virtual void prepareTextureDraw(int mProgram);

    std::shared_ptr<BaseShaderProgramOpenGl> shaderProgram;
    std::string programName;
    int program;

    bool glDataBuffersGenerated = false;
    bool texCoordBufferGenerated = false;
    int mMatrixHandle;
    int originOffsetHandle;
    int positionHandle;
    GLuint vao;
    GLuint vertexBuffer;
    std::vector<GLfloat> vertices;
    int textureUniformHandle;
    int textureCoordinateHandle;
    GLuint textureCoordsBuffer;
    std::vector<GLfloat> textureCoords;
    GLuint indexBuffer;
    std::vector<GLushort> indices;
    Vec3D quadOrigin = Vec3D(0.0, 0.0, 0.0);
    bool is3d = false;

    std::shared_ptr<TextureHolderInterface> textureHolder;
    int texturePointer;
    std::optional<TextureFilterType> textureFilterType = std::nullopt;

    bool usesTextureCoords = false;

    int32_t subdivisionFactor = 0;
    Quad3dD frame = Quad3dD(Vec3D(0.0, 0.0, 0.0), Vec3D(0.0, 0.0, 0.0), Vec3D(0.0, 0.0, 0.0), Vec3D(0.0, 0.0, 0.0));
    RectD textureCoordinates = RectD(0.0, 0.0, 0.0, 0.0);
    double factorHeight = 1.0;
    double factorWidth = 1.0;

    bool ready = false;
    bool textureCoordsReady = false;
    std::recursive_mutex dataMutex;

    bool isMaskInversed = false;
};

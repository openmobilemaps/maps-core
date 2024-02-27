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
#include "opengl_wrapper.h"
#include <mutex>
#include <vector>

class Quad2dOpenGl : public GraphicsObjectInterface,
                     public MaskingObjectInterface,
                     public Quad2dInterface,
                     public std::enable_shared_from_this<Quad2dOpenGl> {
  public:
    Quad2dOpenGl(const std::shared_ptr<::ShaderProgramInterface> &shader);

    ~Quad2dOpenGl(){};

    virtual bool isReady() override;

    virtual void setup(const std::shared_ptr<::RenderingContextInterface> &context) override;

    virtual void clear() override;

    virtual void renderAsMask(const std::shared_ptr<::RenderingContextInterface> &context, const ::RenderPassConfig &renderPass,
                              int64_t mvpMatrix, double screenPixelAsRealMeterFactor) override;

    virtual void render(const std::shared_ptr<::RenderingContextInterface> &context, const ::RenderPassConfig &renderPass,
                        int64_t mvpMatrix, bool isMasked, double screenPixelAsRealMeterFactor) override;

    virtual void setFrame(const ::Quad3dD &frame, const ::RectD &textureCoordinates) override;

    void setSubdivisionFactor(int32_t factor) override;

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

    std::shared_ptr<ShaderProgramInterface> shaderProgram;
    std::string programName;
    int program;

    bool glDataBuffersGenerated = false;
    int mvpMatrixHandle;
    int positionHandle;
    GLuint vertexBuffer;
    std::vector<GLfloat> vertices;
    int textureCoordinateHandle;
    GLuint textureCoordsBuffer;
    std::vector<GLfloat> textureCoords;
    GLuint indexBuffer;
    std::vector<GLushort> indices;

    std::shared_ptr<TextureHolderInterface> textureHolder;
    int texturePointer;

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

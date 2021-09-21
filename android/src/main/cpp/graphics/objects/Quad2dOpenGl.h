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

    virtual void renderAsMask(const std::shared_ptr<::RenderingContextInterface> & context, const ::RenderPassConfig & renderPass, int64_t mvpMatrix, double screenPixelAsRealMeterFactor) override;

    virtual void render(const std::shared_ptr<::RenderingContextInterface> &context, const ::RenderPassConfig &renderPass,
                        int64_t mvpMatrix, bool isMasked, double screenPixelAsRealMeterFactor) override;

    virtual void setFrame(const ::Quad2dD &frame, const ::RectD &textureCoordinates) override;

    virtual void loadTexture(const std::shared_ptr<TextureHolderInterface> &textureHolder) override;

    virtual void removeTexture() override;

    virtual std::shared_ptr<GraphicsObjectInterface> asGraphicsObject() override;

    virtual std::shared_ptr<MaskingObjectInterface> asMaskingObject() override;

  protected:
    virtual void adjustTextureCoordinates();

    virtual void prepareTextureDraw(std::shared_ptr<OpenGlContext> &openGLContext, int mProgram);

    std::shared_ptr<ShaderProgramInterface> shaderProgram;

    std::vector<GLfloat> vertexBuffer;
    std::vector<GLfloat> textureBuffer;
    std::vector<GLubyte> indexBuffer;
    std::vector<GLuint> texturePointer = {0};
    bool textureLoaded = false;

    Quad2dD frame = Quad2dD(Vec2D(0.0, 0.0), Vec2D(0.0, 0.0), Vec2D(0.0, 0.0), Vec2D(0.0, 0.0));
    RectD textureCoordinates = RectD(0.0, 0.0, 0.0, 0.0);
    double factorHeight = 1.0;
    double factorWidth = 1.0;

    bool ready = false;
};

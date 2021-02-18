/*
 * Copyright (c) 2021 Ubique Innovation AG <https://www.ubique.ch>
 *
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this
 *  file, You can obtain one at https://mozilla.org/MPL/2.0/.
 *
 *  SPDX-License-Identifier: MPL-2.0
 */


#ifndef MAPSDK_RECTANGLE2DOPENGL_H
#define MAPSDK_RECTANGLE2DOPENGL_H

#include "GraphicsObjectInterface.h"
#include "OpenGlContext.h"
#include "Rectangle2dInterface.h"
#include "ShaderProgramInterface.h"
#include "opengl_wrapper.h"
#include <vector>

class Rectangle2dOpenGl : public GraphicsObjectInterface,
                          public Rectangle2dInterface,
                          public std::enable_shared_from_this<GraphicsObjectInterface> {
  public:
    Rectangle2dOpenGl(const std::shared_ptr<::ShaderProgramInterface> &shader);

    ~Rectangle2dOpenGl(){};

    virtual bool isReady() override;

    virtual void setup(const std::shared_ptr<::RenderingContextInterface> &context) override;

    virtual void clear() override;

    virtual void render(const std::shared_ptr<::RenderingContextInterface> &context, const ::RenderPassConfig &renderPass,
                        int64_t mvpMatrix) override;

    virtual void setFrame(const ::RectD &frame, const ::RectD &textureCoordinates) override;

    virtual void loadTexture(const std::shared_ptr<TextureHolderInterface> &textureHolder) override;

    virtual void removeTexture() override;

    virtual std::shared_ptr<GraphicsObjectInterface> asGraphicsObject() override;

  protected:
    virtual void adjustTextureCoordinates();

    virtual void prepareTextureDraw(std::shared_ptr<OpenGlContext> &openGLContext, int mProgram);

    std::shared_ptr<ShaderProgramInterface> shaderProgram;

    std::vector<GLfloat> vertexBuffer;
    std::vector<GLfloat> textureBuffer;
    std::vector<GLubyte> indexBuffer;
    std::vector<GLuint> texturePointer = {0};
    bool textureLoaded = false;

    RectD frame = RectD(0.0, 0.0, 0.0, 0.0);
    RectD textureCoordinates = RectD(0.0, 0.0, 0.0, 0.0);
    double factorHeight = 1.0;
    double factorWidth = 1.0;

    bool ready = false;
};

#endif // MAPSDK_RECTANGLE2DOPENGL_H

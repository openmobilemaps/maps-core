/*
 * Copyright (c) 2021 Ubique Innovation AG <https://www.ubique.ch>
 *
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this
 *  file, You can obtain one at https://mozilla.org/MPL/2.0/.
 *
 *  SPDX-License-Identifier: MPL-2.0
 */

#include "Quad2dOpenGl.h"
#include "OpenGlHelper.h"
#include "TextureHolderInterface.h"

Quad2dOpenGl::Quad2dOpenGl(const std::shared_ptr<::ShaderProgramInterface> &shader)
    : shaderProgram(shader) {}

bool Quad2dOpenGl::isReady() { return ready; }

std::shared_ptr<GraphicsObjectInterface> Quad2dOpenGl::asGraphicsObject() { return shared_from_this(); }

void Quad2dOpenGl::clear() {
    removeTexture();
    ready = false;
}

void Quad2dOpenGl::setFrame(const Quad2dD &frame, const RectD &textureCoordinates) {
    this->frame = frame;
    this->textureCoordinates = textureCoordinates;
    ready = false;
}

void Quad2dOpenGl::setup(const std::shared_ptr<::RenderingContextInterface> &context) {
    if (ready)
        return;

    float frameZ = 0;
    vertexBuffer = {
        (float)frame.topLeft.x,     (float)frame.topLeft.y,     frameZ,
        (float)frame.bottomLeft.x,  (float)frame.bottomLeft.y,  frameZ,
        (float)frame.bottomRight.x, (float)frame.bottomRight.y, frameZ,
        (float)frame.topRight.x,    (float)frame.topRight.y,    frameZ,
    };
    indexBuffer = {
        0, 1, 2, 0, 2, 3,
    };
    adjustTextureCoordinates();

    std::shared_ptr<OpenGlContext> openGlContext = std::static_pointer_cast<OpenGlContext>(context);
    if (openGlContext->getProgram(shaderProgram->getProgramName()) == 0) {
        shaderProgram->setupProgram(openGlContext);
    }

    ready = true;
}

void Quad2dOpenGl::loadTexture(const std::shared_ptr<TextureHolderInterface> &textureHolder) {
    glGenTextures(1, (unsigned int *)&texturePointer[0]);

    if (textureHolder != nullptr) {
        glBindTexture(GL_TEXTURE_2D, (unsigned int)texturePointer[0]);

        textureHolder->attachToGraphics();

        // Set filtering
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        factorHeight = textureHolder->getImageHeight() * 1.0f / textureHolder->getTextureHeight();
        factorWidth = textureHolder->getImageWidth() * 1.0f / textureHolder->getTextureWidth();

        adjustTextureCoordinates();

        glBindTexture(GL_TEXTURE_2D, 0);

        textureLoaded = true;
    }
}

void Quad2dOpenGl::removeTexture() {
    glDeleteTextures(1, &texturePointer[0]);
    texturePointer = std::vector<GLuint>(1, 0);
    textureLoaded = false;
}

void Quad2dOpenGl::adjustTextureCoordinates() {
    float tMinX = factorWidth * textureCoordinates.x;
    float tMaxX = factorWidth * (textureCoordinates.x + textureCoordinates.width);
    float tMinY = factorHeight * textureCoordinates.y;
    float tMaxY = factorHeight * (textureCoordinates.y + textureCoordinates.height);

    textureBuffer = {tMinX, tMinY, tMinX, tMaxY, tMaxX, tMaxY, tMaxX, tMinY};
}

void Quad2dOpenGl::render(const std::shared_ptr<::RenderingContextInterface> &context, const RenderPassConfig &renderPass,
                          int64_t mvpMatrix, double screenPixelAsRealMeterFactor) {
    if (!ready)
        return;

    std::shared_ptr<OpenGlContext> openGlContext = std::static_pointer_cast<OpenGlContext>(context);

    int mProgram = openGlContext->getProgram(shaderProgram->getProgramName());

    // Add program to OpenGL environment

    glUseProgram(mProgram);
    OpenGlHelper::checkGlError("glUseProgram RectangleOpenGl");

    if (textureLoaded)
        prepareTextureDraw(openGlContext, mProgram);

    int mTextureCoordinateHandle = glGetAttribLocation(mProgram, "texCoordinate");
    OpenGlHelper::checkGlError("glGetAttribLocation texCoordinate");

    glEnableVertexAttribArray(mTextureCoordinateHandle);
    OpenGlHelper::checkGlError("glEnableVertexAttribArray");

    glVertexAttribPointer(mTextureCoordinateHandle, 2, GL_FLOAT, false, 0, &textureBuffer[0]);
    OpenGlHelper::checkGlError("glVertexAttribPointer tex");

    shaderProgram->preRender(context);

    // get handle to vertex shader's vPosition member
    int mPositionHandle = glGetAttribLocation(mProgram, "vPosition");
    OpenGlHelper::checkGlError("glGetAttribLocation");

    // Enable a handle to the triangle vertices
    glEnableVertexAttribArray(mPositionHandle);

    // Prepare the triangle coordinate data
    glVertexAttribPointer(mPositionHandle, 3, GL_FLOAT, false, 12, &vertexBuffer[0]);

    // get handle to shape's transformation matrix
    int mMVPMatrixHandle = glGetUniformLocation(mProgram, "uMVPMatrix");
    OpenGlHelper::checkGlError("glGetUniformLocation");

    // Apply the projection and view transformation
    glUniformMatrix4fv(mMVPMatrixHandle, 1, false, (GLfloat *)mvpMatrix);
    OpenGlHelper::checkGlError("glUniformMatrix4fv");

    // Enable blending
    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

    // Draw the triangle
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_BYTE, &indexBuffer[0]);

    OpenGlHelper::checkGlError("glDrawElements");

    // Disable vertex array
    glDisableVertexAttribArray(mPositionHandle);

    glDisable(GL_BLEND);
}

void Quad2dOpenGl::prepareTextureDraw(std::shared_ptr<OpenGlContext> &openGLContext, int mProgram) {
    int mTextureUniformHandle = glGetUniformLocation(mProgram, "u_Texture");

    // Set the active texture unit to texture unit 0.
    glActiveTexture(GL_TEXTURE0);

    // Bind the texture to this unit.
    glBindTexture(GL_TEXTURE_2D, (unsigned int)texturePointer[0]);

    // Tell the texture uniform sampler to use this texture in the shader by binding to texture unit 0.
    glUniform1i(mTextureUniformHandle, 0);
}

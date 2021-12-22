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

std::shared_ptr<GraphicsObjectInterface> Quad2dOpenGl::asGraphicsObject() {
    return shared_from_this();
}

std::shared_ptr<MaskingObjectInterface> Quad2dOpenGl::asMaskingObject() {
    return shared_from_this();
}

void Quad2dOpenGl::clear() {
    removeGlBuffers();
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
    vertices = {
        (float)frame.topLeft.x,     (float)frame.topLeft.y,     frameZ,
        (float)frame.bottomLeft.x,  (float)frame.bottomLeft.y,  frameZ,
        (float)frame.bottomRight.x, (float)frame.bottomRight.y, frameZ,
        (float)frame.topRight.x,    (float)frame.topRight.y,    frameZ,
    };
    indices = {
        0, 1, 2, 0, 2, 3,
    };
    adjustTextureCoordinates();

    std::shared_ptr<OpenGlContext> openGlContext = std::static_pointer_cast<OpenGlContext>(context);
    if (openGlContext->getProgram(shaderProgram->getProgramName()) == 0) {
        shaderProgram->setupProgram(openGlContext);
    }

    prepareGlData(openGlContext);
    prepareTextureCoordsGlData(openGlContext);

    ready = true;
}

void Quad2dOpenGl::prepareGlData(const std::shared_ptr<OpenGlContext> &openGlContext) {
    int mProgram = openGlContext->getProgram(shaderProgram->getProgramName());
    glUseProgram(mProgram);

    positionHandle = glGetAttribLocation(mProgram, "vPosition");
    glGenBuffers(1, &vertexBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * vertices.size(), &vertices[0], GL_STATIC_DRAW);
    OpenGlHelper::checkGlError("Setup vPosition buffer");

    glBindBuffer(GL_ARRAY_BUFFER, 0);

    glGenBuffers(1, &indexBuffer);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuffer);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLubyte) * indices.size(), &indices[0], GL_STATIC_DRAW);
    OpenGlHelper::checkGlError("Setup index buffer");
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);


    mvpMatrixHandle = glGetUniformLocation(mProgram, "uMVPMatrix");
    OpenGlHelper::checkGlError("glGetUniformLocation uMVPMatrix");
}

void Quad2dOpenGl::prepareTextureCoordsGlData(const std::shared_ptr<OpenGlContext> &openGlContext) {
    int mProgram = openGlContext->getProgram(shaderProgram->getProgramName());
    glUseProgram(mProgram);

    textureCoordinateHandle = glGetAttribLocation(mProgram, "texCoordinate");
    glGenBuffers(1, &textureCoordsBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, textureCoordsBuffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * textureCoords.size(), &textureCoords[0], GL_STATIC_DRAW);
    OpenGlHelper::checkGlError("Setup texCoordinate buffer");

    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void Quad2dOpenGl::removeGlBuffers() {
    glDeleteBuffers(1, &vertexBuffer);
    glDeleteBuffers(1, &textureCoordsBuffer);
    glDeleteBuffers(1, &indexBuffer);
}


void Quad2dOpenGl::loadTexture(const std::shared_ptr<::RenderingContextInterface> & context, const std::shared_ptr<TextureHolderInterface> &textureHolder) {
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

        std::shared_ptr<OpenGlContext> openGlContext = std::static_pointer_cast<OpenGlContext>(context);
        prepareTextureCoordsGlData(openGlContext);

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

    textureCoords = {tMinX, tMinY, tMinX, tMaxY, tMaxX, tMaxY, tMaxX, tMinY};
}

void Quad2dOpenGl::renderAsMask(const std::shared_ptr<::RenderingContextInterface> &context, const RenderPassConfig &renderPass,
                                int64_t mvpMatrix, double screenPixelAsRealMeterFactor) {
    glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
    render(context, renderPass, mvpMatrix, false, screenPixelAsRealMeterFactor);
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
}

void Quad2dOpenGl::render(const std::shared_ptr<::RenderingContextInterface> &context, const RenderPassConfig &renderPass,
                          int64_t mvpMatrix, bool isMasked, double screenPixelAsRealMeterFactor) {
    if (!ready)
        return;

    if (isMasked) {
        glStencilFunc(GL_EQUAL, 128, 128);
        glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
    }

    std::shared_ptr<OpenGlContext> openGlContext = std::static_pointer_cast<OpenGlContext>(context);
    int mProgram = openGlContext->getProgram(shaderProgram->getProgramName());
    glUseProgram(mProgram);
    OpenGlHelper::checkGlError("glUseProgram RectangleOpenGl");

    if (textureLoaded) {
        prepareTextureDraw(openGlContext, mProgram);
    }

    glEnableVertexAttribArray(textureCoordinateHandle);
    glBindBuffer(GL_ARRAY_BUFFER, textureCoordsBuffer);
    glVertexAttribPointer(textureCoordinateHandle, 2, GL_FLOAT, false, 0, nullptr);
    OpenGlHelper::checkGlError("glEnableVertexAttribArray texCoordinate");

    shaderProgram->preRender(context);

    // enable vPosition attribs
    glEnableVertexAttribArray(positionHandle);
    glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
    glVertexAttribPointer(positionHandle, 3, GL_FLOAT, false, 0, nullptr);
    OpenGlHelper::checkGlError("glEnableVertexAttribArray positionHandle");

    glBindBuffer(GL_ARRAY_BUFFER, 0);

    // Apply the projection and view transformation
    glUniformMatrix4fv(mvpMatrixHandle, 1, false, (GLfloat *)mvpMatrix);
    OpenGlHelper::checkGlError("glUniformMatrix4fv");

    // Enable blending
    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

    // Draw the triangles
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuffer);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_BYTE, nullptr);

    OpenGlHelper::checkGlError("glDrawElements");

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    // Disable vertex array
    glDisableVertexAttribArray(positionHandle);

    if (textureLoaded) {
        glDisableVertexAttribArray(textureCoordinateHandle);
    }

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

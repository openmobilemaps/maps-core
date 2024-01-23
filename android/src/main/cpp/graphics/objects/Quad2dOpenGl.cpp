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
#include "Logger.h"
#include "OpenGlHelper.h"
#include "TextureHolderInterface.h"

Quad2dOpenGl::Quad2dOpenGl(const std::shared_ptr<::ShaderProgramInterface> &shader)
    : shaderProgram(shader) {}

bool Quad2dOpenGl::isReady() { return ready && (!usesTextureCoords || textureHolder); }

std::shared_ptr<GraphicsObjectInterface> Quad2dOpenGl::asGraphicsObject() { return shared_from_this(); }

std::shared_ptr<MaskingObjectInterface> Quad2dOpenGl::asMaskingObject() { return shared_from_this(); }

void Quad2dOpenGl::clear() {
    std::lock_guard<std::recursive_mutex> lock(dataMutex);
    if (ready) {
        removeGlBuffers();
    }
    if (textureCoordsReady) {
        removeTextureCoordsGlBuffers();
    }
    if (textureHolder) {
        removeTexture();
    }
    ready = false;
}

void Quad2dOpenGl::setIsInverseMasked(bool inversed) { isMaskInversed = inversed; }

void Quad2dOpenGl::setFrame(const Quad2dD &frame, const RectD &textureCoordinates) {
    std::lock_guard<std::recursive_mutex> lock(dataMutex);
    ready = false;
    this->frame = frame;
    this->textureCoordinates = textureCoordinates;
}

void Quad2dOpenGl::setup(const std::shared_ptr<::RenderingContextInterface> &context) {
    std::lock_guard<std::recursive_mutex> lock(dataMutex);
    if (ready) {
        return;
    }

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
    programName = shaderProgram->getProgramName();
    program = openGlContext->getProgram(programName);
    if (program == 0) {
        shaderProgram->setupProgram(openGlContext);
        program = openGlContext->getProgram(programName);
    }

    prepareGlData(program);
    prepareTextureCoordsGlData(program);

    ready = true;
}

void Quad2dOpenGl::prepareGlData(int program) {
    glUseProgram(program);

    positionHandle = glGetAttribLocation(program, "vPosition");
    if (!glDataBuffersGenerated) {
        glGenBuffers(1, &vertexBuffer);
    }
    glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * vertices.size(), &vertices[0], GL_STATIC_DRAW);

    glBindBuffer(GL_ARRAY_BUFFER, 0);

    if (!glDataBuffersGenerated) {
        glGenBuffers(1, &indexBuffer);
    }
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuffer);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLubyte) * indices.size(), &indices[0], GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    mvpMatrixHandle = glGetUniformLocation(program, "uMVPMatrix");
    glDataBuffersGenerated = true;
}

void Quad2dOpenGl::prepareTextureCoordsGlData(int program) {
    glUseProgram(program);

    if (textureCoordsReady) {
        removeTextureCoordsGlBuffers();
    }

    textureCoordinateHandle = glGetAttribLocation(program, "texCoordinate");
    if (textureCoordinateHandle < 0) {
        usesTextureCoords = false;
        return;
    }
    glGenBuffers(1, &textureCoordsBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, textureCoordsBuffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * textureCoords.size(), &textureCoords[0], GL_STATIC_DRAW);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    usesTextureCoords = true;
    textureCoordsReady = true;
}

void Quad2dOpenGl::removeGlBuffers() {
    if (glDataBuffersGenerated) {
        glDeleteBuffers(1, &vertexBuffer);
        glDeleteBuffers(1, &indexBuffer);
        glDataBuffersGenerated = false;
    }
}

void Quad2dOpenGl::removeTextureCoordsGlBuffers() {
    if (textureCoordsReady) {
        glDeleteBuffers(1, &textureCoordsBuffer);
        textureCoordsReady = false;
    }
}

void Quad2dOpenGl::loadTexture(const std::shared_ptr<::RenderingContextInterface> &context,
                               const std::shared_ptr<TextureHolderInterface> &textureHolder) {
    std::lock_guard<std::recursive_mutex> lock(dataMutex);
    if (this->textureHolder != nullptr) {
        removeTexture();
    }

    if (textureHolder != nullptr) {
        texturePointer = textureHolder->attachToGraphics();

        factorHeight = textureHolder->getImageHeight() * 1.0f / textureHolder->getTextureHeight();
        factorWidth = textureHolder->getImageWidth() * 1.0f / textureHolder->getTextureWidth();
        adjustTextureCoordinates();

        if (ready) {
            prepareTextureCoordsGlData(program);
        }
        this->textureHolder = textureHolder;
    }
}

void Quad2dOpenGl::removeTexture() {
    std::lock_guard<std::recursive_mutex> lock(dataMutex);
    if (textureHolder) {
        textureHolder->clearFromGraphics();
        textureHolder = nullptr;
        texturePointer = -1;
        if (textureCoordsReady) {
            removeTextureCoordsGlBuffers();
        }
    }
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
    std::lock_guard<std::recursive_mutex> lock(dataMutex);
    if (!ready || (usesTextureCoords && !textureCoordsReady))
        return;

    glUseProgram(program);

    GLuint stencilMask = 0;
    GLuint validTarget = 0;
    GLenum zpass = GL_KEEP;
    if (isMasked) {
        stencilMask += 128;
        validTarget = isMaskInversed ? 0 : 128;
    }
    if (renderPass.isPassMasked) {
        stencilMask += 127;
        zpass = GL_INCR;
    }

    if (stencilMask != 0) {
        glStencilFunc(GL_EQUAL, validTarget, stencilMask);
        glStencilOp(GL_KEEP, GL_KEEP, zpass);
    }

    if (usesTextureCoords) {
        prepareTextureDraw(program);

        glEnableVertexAttribArray(textureCoordinateHandle);
        glBindBuffer(GL_ARRAY_BUFFER, textureCoordsBuffer);
        glVertexAttribPointer(textureCoordinateHandle, 2, GL_FLOAT, false, 0, nullptr);
    }

    shaderProgram->preRender(context);

    // enable vPosition attribs
    glEnableVertexAttribArray(positionHandle);
    glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
    glVertexAttribPointer(positionHandle, 3, GL_FLOAT, false, 0, nullptr);

    glBindBuffer(GL_ARRAY_BUFFER, 0);

    // Apply the projection and view transformation
    glUniformMatrix4fv(mvpMatrixHandle, 1, false, (GLfloat *)mvpMatrix);

    // Draw the triangles
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuffer);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_BYTE, nullptr);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    // Disable vertex array
    glDisableVertexAttribArray(positionHandle);

    if (textureHolder) {
        glDisableVertexAttribArray(textureCoordinateHandle);
    }

    glDisable(GL_BLEND);
}

void Quad2dOpenGl::prepareTextureDraw(int program) {
    if (!textureHolder) {
        return;
    }

    // Set the active texture unit to texture unit 0.
    glActiveTexture(GL_TEXTURE0);

    // Bind the texture to this unit.
    glBindTexture(GL_TEXTURE_2D, (unsigned int)texturePointer);

    // Tell the texture uniform sampler to use this texture in the shader by binding to texture unit 0.
    int textureUniformHandle = glGetUniformLocation(program, "textureSampler");
    glUniform1i(textureUniformHandle, 0);
}

void Quad2dOpenGl::setDebugLabel(const std::string &label) {
    // not used
}

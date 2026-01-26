/*
 * Copyright (c) 2021 Ubique Innovation AG <https://www.ubique.ch>
 *
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this
 *  file, You can obtain one at https://mozilla.org/MPL/2.0/.
 *
 *  SPDX-License-Identifier: MPL-2.0
 */

#include "Quad2dTessellatedOpenGl.h"
#include "TextureHolderInterface.h"
#include "TextureFilterType.h"
#include <cmath>
#include "Logger.h"

Quad2dTessellatedOpenGl::Quad2dTessellatedOpenGl(const std::shared_ptr<::BaseShaderProgramOpenGl> &shader)
    : shaderProgram(shader) {}

bool Quad2dTessellatedOpenGl::isReady() { return ready && (!usesTextureCoords || textureHolder); }

std::shared_ptr<GraphicsObjectInterface> Quad2dTessellatedOpenGl::asGraphicsObject() { return shared_from_this(); }

std::shared_ptr<MaskingObjectInterface> Quad2dTessellatedOpenGl::asMaskingObject() { return shared_from_this(); }

void Quad2dTessellatedOpenGl::clear() {
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

void Quad2dTessellatedOpenGl::setIsInverseMasked(bool inversed) { isMaskInversed = inversed; }

void Quad2dTessellatedOpenGl::setFrame(const Quad3dD &frame, const RectD &textureCoordinates, const Vec3D &origin, bool is3d) {
    std::lock_guard<std::recursive_mutex> lock(dataMutex);
    ready = false;
    this->frame = frame;
    this->textureCoordinates = textureCoordinates;
    this->quadOrigin = origin;
    this->is3d = is3d;
}

void Quad2dTessellatedOpenGl::setSubdivisionFactor(int32_t factor) {
    std::lock_guard<std::recursive_mutex> lock(dataMutex);
    if (factor != subdivisionFactor) {
        subdivisionFactor = factor;
    }
}

void Quad2dTessellatedOpenGl::setMinMagFilter(TextureFilterType filterType) {
    std::lock_guard<std::recursive_mutex> lock(dataMutex);
    textureFilterType = filterType;
}

void Quad2dTessellatedOpenGl::setup(const std::shared_ptr<::RenderingContextInterface> &context) {
    std::lock_guard<std::recursive_mutex> lock(dataMutex);
    if (ready) {
        return;
    }

    computeGeometry(false);

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

void Quad2dTessellatedOpenGl::computeGeometry(bool texCoordsOnly) {
    // Data mutex covered by caller Quad2dTessellatedOpenGL::setup()
    if (!texCoordsOnly) {
        if (is3d) {
            vertices = {
                    // Position
                    (float) (1.0 * std::sin(frame.topLeft.y) * std::cos(frame.topLeft.x) - quadOrigin.x),
                    (float) (1.0 * cos(frame.topLeft.y) - quadOrigin.y),
                    (float) (-1.0 * std::sin(frame.topLeft.x) * std::sin(frame.topLeft.y) - quadOrigin.z),
                    // Frame Coord
                    (float) (frame.topLeft.x),
                    (float) (frame.topLeft.y),

                    // Position
                    (float) (1.0 * std::sin(frame.topRight.y) * std::cos(frame.topRight.x) - quadOrigin.x),
                    (float) (1.0 * cos(frame.topRight.y) - quadOrigin.y),
                    (float) (-1.0 * std::sin(frame.topRight.x) * std::sin(frame.topRight.y) - quadOrigin.z),
                    // Frame Coord
                    (float) (frame.topRight.x),
                    (float) (frame.topRight.y),

                    // Position
                    (float) (1.0 * std::sin(frame.bottomLeft.y) * std::cos(frame.bottomLeft.x) - quadOrigin.x),
                    (float) (1.0 * cos(frame.bottomLeft.y) - quadOrigin.y),
                    (float) (-1.0 * std::sin(frame.bottomLeft.x) * std::sin(frame.bottomLeft.y) - quadOrigin.z),
                    // Frame Coord
                    (float) (frame.bottomLeft.x),
                    (float) (frame.bottomLeft.y),

                    // Position
                    (float) (1.0 * std::sin(frame.bottomRight.y) * std::cos(frame.bottomRight.x) - quadOrigin.x),
                    (float) (1.0 * cos(frame.bottomRight.y) - quadOrigin.y),
                    (float) (-1.0 * std::sin(frame.bottomRight.x) * std::sin(frame.bottomRight.y) - quadOrigin.z),
                    // Frame Coord
                    (float) (frame.bottomRight.x),
                    (float) (frame.bottomRight.y),
            };
        } else {
            vertices = {
                    // Position
                    (float) (frame.topLeft.x - quadOrigin.x),
                    (float) (frame.topLeft.y - quadOrigin.y),
                    (float) (-quadOrigin.z),
                    // Frame Coord
                    (float) (frame.topLeft.x),
                    (float) (frame.topLeft.y),

                    // Position
                    (float) (frame.topRight.x - quadOrigin.x),
                    (float) (frame.topRight.y - quadOrigin.y),
                    (float) (-quadOrigin.z),
                    // Frame Coord
                    (float) (frame.topRight.x),
                    (float) (frame.topRight.y),

                    // Position
                    (float) (frame.bottomLeft.x - quadOrigin.x),
                    (float) (frame.bottomLeft.y - quadOrigin.y),
                    (float) (-quadOrigin.z),
                    // Frame Coord
                    (float) (frame.bottomLeft.x),
                    (float) (frame.bottomLeft.y),

                    // Position
                    (float) (frame.bottomRight.x - quadOrigin.x),
                    (float) (frame.bottomRight.y - quadOrigin.y),
                    (float) (-quadOrigin.z),
                    // Frame Coord
                    (float) (frame.bottomRight.x),
                    (float) (frame.bottomRight.y),
            };
        }
    }

    float tMinX = factorWidth * textureCoordinates.x;
    float tMaxX = factorWidth * (textureCoordinates.x + textureCoordinates.width);
    float tMinY = factorHeight * textureCoordinates.y;
    float tMaxY = factorHeight * (textureCoordinates.y + textureCoordinates.height);

    textureCoords = {tMinX, tMinY, tMaxX, tMinY, tMinX, tMaxY, tMaxX, tMaxY};
}

void Quad2dTessellatedOpenGl::prepareGlData(int program) {
    glUseProgram(program);

    if (!glDataBuffersGenerated) {
        glGenVertexArrays(1, &vao);
    }
    glBindVertexArray(vao);

    positionHandle = glGetAttribLocation(program, "vPosition");
    frameCoordHandle = glGetAttribLocation(program, "vFrameCoord");

    if (!glDataBuffersGenerated) {
        glGenBuffers(1, &vertexBuffer);
    }
    glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * vertices.size(), &vertices[0], GL_STATIC_DRAW);

    size_t stride = sizeof(GLfloat) * 5;
    glEnableVertexAttribArray(positionHandle);
    glVertexAttribPointer(positionHandle, 3, GL_FLOAT, false, stride, nullptr);
    glEnableVertexAttribArray(frameCoordHandle);
    glVertexAttribPointer(frameCoordHandle, 2, GL_FLOAT, false, stride, (float*)(sizeof(GLfloat) * 3));

    glBindBuffer(GL_ARRAY_BUFFER, 0);

    glBindVertexArray(0);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    mMatrixHandle = glGetUniformLocation(program, "umMatrix");
    originOffsetHandle = glGetUniformLocation(program, "uOriginOffset");
    subdivisionFactorHandle = glGetUniformLocation(program, "uSubdivisionFactor");
    originHandle = glGetUniformLocation(program, "uOrigin");
    is3dHandle = glGetUniformLocation(program, "uIs3d");

    glDataBuffersGenerated = true;
}

void Quad2dTessellatedOpenGl::prepareTextureCoordsGlData(int program) {
    glUseProgram(program);
    glBindVertexArray(vao);

    textureCoordinateHandle = glGetAttribLocation(program, "texCoordinate");
    if (textureCoordinateHandle < 0) {
        usesTextureCoords = false;
        return;
    }

    textureUniformHandle = glGetUniformLocation(program, "textureSampler");

    if (!texCoordBufferGenerated) {
        glGenBuffers(1, &textureCoordsBuffer);
        texCoordBufferGenerated = true;
    }

    glBindBuffer(GL_ARRAY_BUFFER, textureCoordsBuffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * textureCoords.size(), &textureCoords[0], GL_STATIC_DRAW);

    glEnableVertexAttribArray(textureCoordinateHandle);
    glVertexAttribPointer(textureCoordinateHandle, 2, GL_FLOAT, false, 0, nullptr);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    usesTextureCoords = true;
    textureCoordsReady = true;

    glBindVertexArray(0);
}

void Quad2dTessellatedOpenGl::removeGlBuffers() {
    if (glDataBuffersGenerated) {
        glDeleteBuffers(1, &vertexBuffer);
        glDeleteVertexArrays(1, &vao);
        glDataBuffersGenerated = false;
    }
}

void Quad2dTessellatedOpenGl::removeTextureCoordsGlBuffers() {
    if (textureCoordsReady) {
        if (texCoordBufferGenerated) {
            glDeleteBuffers(1, &textureCoordsBuffer);
            texCoordBufferGenerated = false;
        }
        textureCoordsReady = false;
    }
}

void Quad2dTessellatedOpenGl::loadTexture(const std::shared_ptr<::RenderingContextInterface> &context,
                               const std::shared_ptr<TextureHolderInterface> &textureHolder) {
    std::lock_guard<std::recursive_mutex> lock(dataMutex);
    if (this->textureHolder == textureHolder) {
        return;
    }

    if (this->textureHolder != nullptr) {
        removeTexture();
    }

    if (textureHolder != nullptr) {
        texturePointer = textureHolder->attachToGraphics();

        factorHeight = textureHolder->getImageHeight() * 1.0f / textureHolder->getTextureHeight();
        factorWidth = textureHolder->getImageWidth() * 1.0f / textureHolder->getTextureWidth();
        computeGeometry(true);

        if (ready) {
            prepareTextureCoordsGlData(program);
        }
        this->textureHolder = textureHolder;
    }
}

void Quad2dTessellatedOpenGl::removeTexture() {
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

void Quad2dTessellatedOpenGl::renderAsMask(const std::shared_ptr<::RenderingContextInterface> &context, const RenderPassConfig &renderPass,
                                int64_t vpMatrix, int64_t mMatrix, const ::Vec3D &origin,
                                double screenPixelAsRealMeterFactor, bool isScreenSpaceCoords) {
    glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
    render(context, renderPass, vpMatrix, mMatrix, origin, false, screenPixelAsRealMeterFactor, isScreenSpaceCoords);
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
}

void Quad2dTessellatedOpenGl::render(const std::shared_ptr<::RenderingContextInterface> &context, const RenderPassConfig &renderPass,
                          int64_t vpMatrix, int64_t mMatrix, const ::Vec3D &origin, bool isMasked,
                          double screenPixelAsRealMeterFactor, bool isScreenSpaceCoords) {
    std::lock_guard<std::recursive_mutex> lock(dataMutex);
    if (!ready || (usesTextureCoords && !textureCoordsReady) || !shaderProgram->isRenderable()) {
        return;
    }

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

    glUseProgram(program);
    glBindVertexArray(vao);

    if (usesTextureCoords) {
        prepareTextureDraw(program);
    }

    shaderProgram->preRender(context, isScreenSpaceCoords);

    if(shaderProgram->usesModelMatrix()) {
        glUniformMatrix4fv(mMatrixHandle, 1, false, (GLfloat *) mMatrix);
    }

    glUniform4f(originOffsetHandle, quadOrigin.x - origin.x, quadOrigin.y - origin.y, quadOrigin.z - origin.z, 0.0);

    glPatchParameteri(GL_PATCH_VERTICES, 4);

    glUniform1i(subdivisionFactorHandle, pow(2, subdivisionFactor));

    glUniform4f(originHandle, origin.x, origin.y, origin.z, 0.0);

    glUniform1i(is3dHandle, is3d);

    glDrawArrays(GL_PATCHES, 0, 4);

    glBindVertexArray(0);

    glDisable(GL_BLEND);
}

void Quad2dTessellatedOpenGl::prepareTextureDraw(int program) {
    if (!textureHolder) {
        return;
    }

    // Set the active texture unit to texture unit 0.
    glActiveTexture(GL_TEXTURE0);

    // Bind the texture to this unit.
    glBindTexture(GL_TEXTURE_2D, (unsigned int)texturePointer);
    if (textureFilterType.has_value()) {
        GLint filterParam = *textureFilterType == TextureFilterType::LINEAR ? GL_LINEAR : GL_NEAREST;
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filterParam);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filterParam);
    }

    // Tell the texture uniform sampler to use this texture in the shader by binding to texture unit 0.
    glUniform1i(textureUniformHandle, 0);
}

void Quad2dTessellatedOpenGl::setDebugLabel(const std::string &label) {
    // not used
}

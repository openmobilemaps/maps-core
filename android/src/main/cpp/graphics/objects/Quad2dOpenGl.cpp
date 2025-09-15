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
#include "TextureHolderInterface.h"
#include "TextureFilterType.h"
#include <cmath>

Quad2dOpenGl::Quad2dOpenGl(const std::shared_ptr<::BaseShaderProgramOpenGl> &shader)
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

void Quad2dOpenGl::setFrame(const Quad3dD &frame, const RectD &textureCoordinates, const Vec3D &origin, bool is3d) {
    std::lock_guard<std::recursive_mutex> lock(dataMutex);
    ready = false;
    this->frame = frame;
    this->textureCoordinates = textureCoordinates;
    this->quadOrigin = origin;
    this->is3d = is3d;
}

void Quad2dOpenGl::setSubdivisionFactor(int32_t factor) {
    if (factor != subdivisionFactor) {
        subdivisionFactor = factor;
        ready = false;
    }
}

void Quad2dOpenGl::setMinMagFilter(TextureFilterType filterType) {
    std::lock_guard<std::recursive_mutex> lock(dataMutex);
    textureFilterType = filterType;
}

void Quad2dOpenGl::setup(const std::shared_ptr<::RenderingContextInterface> &context) {
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

void Quad2dOpenGl::computeGeometry(bool texCoordsOnly) {
    // Data mutex covered by caller Quad2dOpenGL::setup()
    if (subdivisionFactor == 0) {
        if (!texCoordsOnly) {
            if (is3d) {
                vertices = {
                        (float) (1.0 * std::sin(frame.topLeft.y) * std::cos(frame.topLeft.x) - quadOrigin.x),
                        (float) (1.0 * cos(frame.topLeft.y) - quadOrigin.y),
                        (float) (-1.0 * std::sin(frame.topLeft.x) * std::sin(frame.topLeft.y) - quadOrigin.z),

                        (float) (1.0 * std::sin(frame.bottomLeft.y) * std::cos(frame.bottomLeft.x) - quadOrigin.x),
                        (float) (1.0 * cos(frame.bottomLeft.y) - quadOrigin.y),
                        (float) (-1.0 * std::sin(frame.bottomLeft.x) * std::sin(frame.bottomLeft.y) - quadOrigin.z),

                        (float) (1.0 * std::sin(frame.bottomRight.y) * std::cos(frame.bottomRight.x) - quadOrigin.x),
                        (float) (1.0 * cos(frame.bottomRight.y) - quadOrigin.y),
                        (float) (-1.0 * std::sin(frame.bottomRight.x) * std::sin(frame.bottomRight.y) - quadOrigin.z),

                        (float) (1.0 * std::sin(frame.topRight.y) * std::cos(frame.topRight.x) - quadOrigin.x),
                        (float) (1.0 * cos(frame.topRight.y) - quadOrigin.y),
                        (float) (-1.0 * std::sin(frame.topRight.x) * std::sin(frame.topRight.y) - quadOrigin.z),
                };
            } else {
                vertices = {
                        (float) (frame.topLeft.x - quadOrigin.x), (float) (frame.topLeft.y - quadOrigin.y), (float) (-quadOrigin.z),
                        (float) (frame.bottomLeft.x - quadOrigin.x), (float) (frame.bottomLeft.y - quadOrigin.y), (float) (-quadOrigin.z),
                        (float) (frame.bottomRight.x - quadOrigin.x), (float) (frame.bottomRight.y - quadOrigin.y), (float) (-quadOrigin.z),
                        (float) (frame.topRight.x - quadOrigin.x), (float) (frame.topRight.y - quadOrigin.y), (float) (-quadOrigin.z),
                };
            }
            indices = {
                    0, 1, 2, 0, 2, 3,
            };
        }

        float tMinX = factorWidth * textureCoordinates.x;
        float tMaxX = factorWidth * (textureCoordinates.x + textureCoordinates.width);
        float tMinY = factorHeight * textureCoordinates.y;
        float tMaxY = factorHeight * (textureCoordinates.y + textureCoordinates.height);

        textureCoords = {tMinX, tMinY, tMinX, tMaxY, tMaxX, tMaxY, tMaxX, tMinY};

    } else {
        if (!texCoordsOnly) {
            vertices = {};
            indices = {};
        }
        textureCoords = {};



        int32_t numSubd = pow(2.0, subdivisionFactor);
        std::vector<float> deltaRTop = {(float) (frame.topRight.x - frame.topLeft.x),
                                        (float) (frame.topRight.y - frame.topLeft.y),
                                        (float) (frame.topRight.z - frame.topLeft.z)};
        std::vector<float> deltaDLeft = {(float) (frame.bottomLeft.x - frame.topLeft.x),
                                        (float) (frame.bottomLeft.y - frame.topLeft.y),
                                        (float) (frame.bottomLeft.z - frame.topLeft.z)};
        std::vector<float> deltaDRight = {(float) (frame.bottomRight.x - frame.topRight.x),
                                        (float) (frame.bottomRight.y - frame.topRight.y),
                                        (float) (frame.bottomRight.z - frame.topRight.z)};

        float pcR, originX, originY, originZ, pcD, deltaDX, deltaDY, deltaDZ;
        for (int iR = 0; iR <= numSubd; ++iR) {
            pcR = iR / (float) numSubd;
            originX = frame.topLeft.x + pcR * deltaRTop[0];
            originY = frame.topLeft.y + pcR * deltaRTop[1];
            originZ = frame.topLeft.z + pcR * deltaRTop[2];
            for (int iD = 0; iD <= numSubd; ++iD) {
                pcD = iD / (float) numSubd;
                deltaDX = pcD * ((1.0 - pcR) * deltaDLeft[0] + pcR * deltaDRight[0]);
                deltaDY = pcD * ((1.0 - pcR) * deltaDLeft[1] + pcR * deltaDRight[1]);
                deltaDZ = pcD * ((1.0 - pcR) * deltaDLeft[2] + pcR * deltaDRight[2]);

                if (!texCoordsOnly) {
                    double x = originX + deltaDX;
                    double y = originY + deltaDY;
                    double z = is3d ? originZ + deltaDZ : 0.0;
                    if (is3d) {
                        vertices.push_back((float) (1.0 * std::sin(y) * std::cos(x) - quadOrigin.x));
                        vertices.push_back((float) (1.0 * cos(y) - quadOrigin.y));
                        vertices.push_back((float) (-1.0 * std::sin(x) * std::sin(y) - quadOrigin.z));
                    } else {
                        vertices.push_back((float) (x - quadOrigin.x));
                        vertices.push_back((float) (y - quadOrigin.y));
                        vertices.push_back((float) (z - quadOrigin.z));
                    }

                    if (iR < numSubd && iD < numSubd) {
                        int baseInd = iD + (iR * (numSubd + 1));
                        int baseIndNextCol = baseInd + (numSubd + 1);
                        indices.push_back(baseInd);
                        indices.push_back(baseInd + 1);
                        indices.push_back(baseIndNextCol + 1);
                        indices.push_back(baseInd);
                        indices.push_back(baseIndNextCol + 1);
                        indices.push_back(baseIndNextCol);
                    }
                }
                float u = factorWidth * (textureCoordinates.x + pcR * textureCoordinates.width);
                float v = factorHeight * (textureCoordinates.y + pcD * textureCoordinates.height);
                textureCoords.push_back(u);
                textureCoords.push_back(v);

            }
        }
    }
}

void Quad2dOpenGl::prepareGlData(int program) {
    glUseProgram(program);

    if (!glDataBuffersGenerated) {
        glGenVertexArrays(1, &vao);
    }
    glBindVertexArray(vao);

    positionHandle = glGetAttribLocation(program, "vPosition");
    if (!glDataBuffersGenerated) {
        glGenBuffers(1, &vertexBuffer);
    }
    glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * vertices.size(), &vertices[0], GL_STATIC_DRAW);

    // enable vPosition attribs
    glEnableVertexAttribArray(positionHandle);
    glVertexAttribPointer(positionHandle, 3, GL_FLOAT, false, 0, nullptr);

    glBindBuffer(GL_ARRAY_BUFFER, 0);

    if (!glDataBuffersGenerated) {
        glGenBuffers(1, &indexBuffer);
    }
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuffer);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLushort ) * indices.size(), &indices[0], GL_STATIC_DRAW);

    glBindVertexArray(0);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    mMatrixHandle = glGetUniformLocation(program, "umMatrix");
    originOffsetHandle = glGetUniformLocation(program, "uOriginOffset");

    glDataBuffersGenerated = true;
}

void Quad2dOpenGl::prepareTextureCoordsGlData(int program) {
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

void Quad2dOpenGl::removeGlBuffers() {
    if (glDataBuffersGenerated) {
        glDeleteBuffers(1, &vertexBuffer);
        glDeleteBuffers(1, &indexBuffer);
        glDeleteVertexArrays(1, &vao);
        glDataBuffersGenerated = false;
    }
}

void Quad2dOpenGl::removeTextureCoordsGlBuffers() {
    if (textureCoordsReady) {
        if (texCoordBufferGenerated) {
            glDeleteBuffers(1, &textureCoordsBuffer);
            texCoordBufferGenerated = false;
        }
        textureCoordsReady = false;
    }
}

void Quad2dOpenGl::loadTexture(const std::shared_ptr<::RenderingContextInterface> &context,
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

void Quad2dOpenGl::renderAsMask(const std::shared_ptr<::RenderingContextInterface> &context, const RenderPassConfig &renderPass,
                                int64_t vpMatrix, int64_t mMatrix, const ::Vec3D &origin,
                                double screenPixelAsRealMeterFactor, bool isScreenSpaceCoords) {
    glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
    render(context, renderPass, vpMatrix, mMatrix, origin, false, screenPixelAsRealMeterFactor, isScreenSpaceCoords);
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
}
#include "Logger.h"
void Quad2dOpenGl::render(const std::shared_ptr<::RenderingContextInterface> &context, const RenderPassConfig &renderPass,
                          int64_t vpMatrix, int64_t mMatrix, const ::Vec3D &origin, bool isMasked,
                          double screenPixelAsRealMeterFactor, bool isScreenSpaceCoords) {
    std::lock_guard<std::recursive_mutex> lock(dataMutex);
    if (!ready || (usesTextureCoords && !textureCoordsReady) || !shaderProgram->isRenderable())
        return;

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

    // Draw the triangles
    glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_SHORT, nullptr);

    glBindVertexArray(0);

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
    if (textureFilterType.has_value()) {
        GLint filterParam = *textureFilterType == TextureFilterType::LINEAR ? GL_LINEAR : GL_NEAREST;
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filterParam);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filterParam);
    }

    // Tell the texture uniform sampler to use this texture in the shader by binding to texture unit 0.
    glUniform1i(textureUniformHandle, 0);
}

void Quad2dOpenGl::setDebugLabel(const std::string &label) {
    // not used
}

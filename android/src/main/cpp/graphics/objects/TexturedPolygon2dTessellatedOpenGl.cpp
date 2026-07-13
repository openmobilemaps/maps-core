/*
 * Copyright (c) 2021 Ubique Innovation AG <https://www.ubique.ch>
 *
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this
 *  file, You can obtain one at https://mozilla.org/MPL/2.0/.
 *
 *  SPDX-License-Identifier: MPL-2.0
 */

#include "TexturedPolygon2dTessellatedOpenGl.h"
#include "TessellationSettings.h"
#include "TextureHolderInterface.h"
#include <algorithm>
#include <cmath>
#include <cstring>

TexturedPolygon2dTessellatedOpenGl::TexturedPolygon2dTessellatedOpenGl(
    const std::shared_ptr<::TessellatedRasterShaderOpenGl> &shader)
    : shaderProgram(shader) {
    shaderProgram->setTexturedPolygonMode();
}

bool TexturedPolygon2dTessellatedOpenGl::isReady() { return ready && textureAttachment.isAttached(); }

std::shared_ptr<GraphicsObjectInterface> TexturedPolygon2dTessellatedOpenGl::asGraphicsObject() { return shared_from_this(); }

std::shared_ptr<MaskingObjectInterface> TexturedPolygon2dTessellatedOpenGl::asMaskingObject() { return shared_from_this(); }

void TexturedPolygon2dTessellatedOpenGl::setVertices(const ::SharedBytes &vertices_,
                                                     const ::SharedBytes &indices_,
                                                     const ::Vec3D &origin,
                                                     bool is3d) {
    std::lock_guard<std::recursive_mutex> lock(dataMutex);
    ready = false;
    dataReady = false;
    polygonOrigin = origin;
    this->is3d = is3d;

    indices.resize(indices_.elementCount);
    if (indices_.elementCount > 0) {
        std::memcpy(indices.data(), reinterpret_cast<void *>(indices_.address),
                    indices_.elementCount * indices_.bytesPerElement);
    }

    vertices.resize(vertices_.elementCount);
    if (vertices_.elementCount > 0) {
        std::memcpy(vertices.data(), reinterpret_cast<void *>(vertices_.address),
                    vertices_.elementCount * vertices_.bytesPerElement);
    }

    vertexStride = 0;
    textureCoordOffset = 0;
    unscaledTextureCoords.clear();

    int vertexCount = 0;
    for (auto index : indices) {
        vertexCount = std::max(vertexCount, static_cast<int>(index) + 1);
    }
    vertexStride = vertexCount > 0 ? static_cast<int>(vertices.size()) / vertexCount : 0;
    textureCoordOffset = vertexStride >= 12 ? 6 : 4;

    if (vertexStride != 12 || vertices.size() % vertexStride != 0) {
        indices.clear();
        vertices.clear();
        vertexStride = 0;
        return;
    }

    unscaledTextureCoords.reserve(vertexCount * 2);
    for (int i = 0; i < vertexCount; ++i) {
        const int base = i * vertexStride + textureCoordOffset;
        unscaledTextureCoords.push_back(vertices[base]);
        unscaledTextureCoords.push_back(vertices[base + 1]);
    }
    updateScaledTextureCoordinates();

    dataReady = true;
}

void TexturedPolygon2dTessellatedOpenGl::setSubdivisionFactor(int32_t factor) {
    std::lock_guard<std::recursive_mutex> lock(dataMutex);
    subdivisionFactor = factor;
}

void TexturedPolygon2dTessellatedOpenGl::setMinMagFilter(TextureFilterType filterType) {
    std::lock_guard<std::recursive_mutex> lock(dataMutex);
    textureFilterType = filterType;
}

void TexturedPolygon2dTessellatedOpenGl::setup(const std::shared_ptr<::RenderingContextInterface> &context) {
    std::lock_guard<std::recursive_mutex> lock(dataMutex);
    if (ready || !dataReady) {
        return;
    }

    std::shared_ptr<OpenGlContext> openGlContext = std::static_pointer_cast<OpenGlContext>(context);
    programName = shaderProgram->getProgramName();
    program = openGlContext->getProgram(programName);
    if (program == 0) {
        shaderProgram->setupProgram(openGlContext);
        program = openGlContext->getProgram(programName);
    }

    prepareGlData(program);
    ready = true;
}

void TexturedPolygon2dTessellatedOpenGl::prepareGlData(int program) {
    glUseProgram(program);

    if (!glDataBuffersGenerated) {
        glGenVertexArrays(1, &vao);
    }
    glBindVertexArray(vao);

    positionHandle = glGetAttribLocation(program, "vPosition");
    frameCoordHandle = glGetAttribLocation(program, "vFrameCoord");
    textureCoordinateHandle = glGetAttribLocation(program, "texCoordinate");
    skirtOffsetHandle = glGetAttribLocation(program, "vSkirtOffset");

    if (!glDataBuffersGenerated) {
        glGenBuffers(1, &vertexBuffer);
    }
    glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * vertices.size(), vertices.data(), GL_STATIC_DRAW);

    const size_t stride = sizeof(GLfloat) * vertexStride;
    if (positionHandle >= 0) {
        glEnableVertexAttribArray(positionHandle);
        glVertexAttribPointer(positionHandle, 4, GL_FLOAT, false, stride, nullptr);
    }
    if (frameCoordHandle >= 0) {
        glEnableVertexAttribArray(frameCoordHandle);
        glVertexAttribPointer(frameCoordHandle, 2, GL_FLOAT, false, stride, reinterpret_cast<void *>(sizeof(GLfloat) * 4));
    }
    if (textureCoordinateHandle >= 0) {
        glEnableVertexAttribArray(textureCoordinateHandle);
        glVertexAttribPointer(textureCoordinateHandle, 2, GL_FLOAT, false, stride,
                              reinterpret_cast<void *>(sizeof(GLfloat) * textureCoordOffset));
    }
    if (skirtOffsetHandle >= 0) {
        glEnableVertexAttribArray(skirtOffsetHandle);
        glVertexAttribPointer(skirtOffsetHandle, 1, GL_FLOAT, false, stride, reinterpret_cast<void *>(sizeof(GLfloat) * 8));
    }

    glBindBuffer(GL_ARRAY_BUFFER, 0);

    if (!glDataBuffersGenerated) {
        glGenBuffers(1, &indexBuffer);
    }
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuffer);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLushort) * indices.size(), indices.data(), GL_STATIC_DRAW);

    glBindVertexArray(0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    mMatrixHandle = glGetUniformLocation(program, "umMatrix");
    originOffsetHandle = glGetUniformLocation(program, "uOriginOffset");
    subdivisionFactorHandle = glGetUniformLocation(program, "uSubdivisionFactor");
    originHandle = glGetUniformLocation(program, "uOrigin");
    is3dHandle = glGetUniformLocation(program, "uIs3d");
    textureUniformHandle = glGetUniformLocation(program, "textureSampler");
    elevationTextureUniformHandle = glGetUniformLocation(program, "elevationTextureSampler");
    hasElevationTextureHandle = glGetUniformLocation(program, "uHasElevationTexture");

    glDataBuffersGenerated = true;
}

void TexturedPolygon2dTessellatedOpenGl::clear() {
    std::lock_guard<std::recursive_mutex> lock(dataMutex);
    removeGlBuffers();
    removeTexture();
    ready = false;
}

void TexturedPolygon2dTessellatedOpenGl::removeGlBuffers() {
    if (glDataBuffersGenerated) {
        glDeleteBuffers(1, &vertexBuffer);
        glDeleteBuffers(1, &indexBuffer);
        glDeleteVertexArrays(1, &vao);
        glDataBuffersGenerated = false;
    }
}

void TexturedPolygon2dTessellatedOpenGl::loadTexture(const std::shared_ptr<::RenderingContextInterface> &context,
                                                     const std::shared_ptr<TextureHolderInterface> &textureHolder) {
    std::lock_guard<std::recursive_mutex> lock(dataMutex);
    const bool newlyAttached = textureAttachment.attach(textureHolder);
    if (newlyAttached) {
        updateScaledTextureCoordinates();
        if (ready) {
            prepareGlData(program);
        }
    }
}

void TexturedPolygon2dTessellatedOpenGl::loadDualTexture(const std::shared_ptr<::RenderingContextInterface> &context,
                                                         const std::shared_ptr<TextureHolderInterface> &textureHolder,
                                                         const std::shared_ptr<TextureHolderInterface> &elevationHolder) {
    std::lock_guard<std::recursive_mutex> lock(dataMutex);
    const bool textureChanged = textureAttachment.attach(textureHolder);
    const bool elevationChanged = elevationTextureAttachment.attach(elevationHolder);
    if (textureChanged) {
        updateScaledTextureCoordinates();
    }
    if ((textureChanged || elevationChanged) && ready) {
        prepareGlData(program);
    }
}

void TexturedPolygon2dTessellatedOpenGl::removeTexture() {
    std::lock_guard<std::recursive_mutex> lock(dataMutex);
    textureAttachment.clear();
    elevationTextureAttachment.clear();
}

void TexturedPolygon2dTessellatedOpenGl::updateScaledTextureCoordinates() {
    if (vertexStride == 0 || unscaledTextureCoords.empty()) {
        return;
    }

    const float widthFactor = textureAttachment.isSet() ? textureAttachment.widthFactor() : 1.0f;
    const float heightFactor = textureAttachment.isSet() ? textureAttachment.heightFactor() : 1.0f;
    for (size_t vertexIndex = 0; vertexIndex * 2 + 1 < unscaledTextureCoords.size(); ++vertexIndex) {
        const size_t base = vertexIndex * vertexStride + textureCoordOffset;
        vertices[base] = widthFactor * unscaledTextureCoords[vertexIndex * 2];
        vertices[base + 1] = heightFactor * unscaledTextureCoords[vertexIndex * 2 + 1];
    }
}

void TexturedPolygon2dTessellatedOpenGl::renderAsMask(const std::shared_ptr<::RenderingContextInterface> &context,
                                                      const RenderPassConfig &renderPass, int64_t vpMatrix,
                                                      int64_t mMatrix, const ::Vec3D &origin,
                                                      double screenPixelAsRealMeterFactor, bool isScreenSpaceCoords) {
    glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
    render(context, renderPass, vpMatrix, mMatrix, origin, false, screenPixelAsRealMeterFactor, isScreenSpaceCoords);
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
}

void TexturedPolygon2dTessellatedOpenGl::render(const std::shared_ptr<::RenderingContextInterface> &context,
                                                const RenderPassConfig &renderPass, int64_t vpMatrix,
                                                int64_t mMatrix, const ::Vec3D &origin, bool isMasked,
                                                double screenPixelAsRealMeterFactor, bool isScreenSpaceCoords) {
    std::lock_guard<std::recursive_mutex> lock(dataMutex);
    if (!isReady() || !shaderProgram->isRenderable()) {
        return;
    }

    const bool hasElevationTexture = elevationTextureAttachment.isAttached();
    if (hasElevationTexture) {
        enableDepthTest();
    } else {
        disableDepthTest();
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

    GLboolean cullFaceEnabled = GL_FALSE;
    GLint cullFaceMode = GL_BACK;
    if (hasElevationTexture) {
        cullFaceEnabled = glIsEnabled(GL_CULL_FACE);
        glGetIntegerv(GL_CULL_FACE_MODE, &cullFaceMode);
        glDisable(GL_CULL_FACE);
    }

    prepareTextureDraw(program);
    shaderProgram->preRender(context, isScreenSpaceCoords);

    if (shaderProgram->usesModelMatrix()) {
        glUniformMatrix4fv(mMatrixHandle, 1, false, reinterpret_cast<GLfloat *>(mMatrix));
    }

    glUniform4f(originOffsetHandle, polygonOrigin.x - origin.x, polygonOrigin.y - origin.y, polygonOrigin.z - origin.z, 0.0);
#if HARDWARE_TESSELLATION_SUPPORTED
    glPatchParameteri(GL_PATCH_VERTICES, 3);
#endif
    glUniform1i(subdivisionFactorHandle, std::pow(2, subdivisionFactor));
    glUniform4f(originHandle, origin.x, origin.y, origin.z, 0.0);
    glUniform1i(is3dHandle, is3d);
    glUniform1i(hasElevationTextureHandle, hasElevationTexture);

    glDrawElements(GL_PATCHES, indices.size(), GL_UNSIGNED_SHORT, nullptr);

    if (hasElevationTexture) {
        if (cullFaceEnabled) {
            glEnable(GL_CULL_FACE);
            glCullFace(cullFaceMode);
        } else {
            glDisable(GL_CULL_FACE);
        }
    }

    glBindVertexArray(0);
    glDisable(GL_BLEND);
    if (hasElevationTexture) {
        disableDepthTest();
    }
}

void TexturedPolygon2dTessellatedOpenGl::prepareTextureDraw(int program) {
    if (!textureAttachment.isAttached()) {
        return;
    }

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, textureAttachment.texture());
    if (textureFilterType.has_value()) {
        GLint filterParam = *textureFilterType == TextureFilterType::LINEAR ? GL_LINEAR : GL_NEAREST;
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filterParam);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filterParam);
    }
    glUniform1i(textureUniformHandle, 0);

    if (elevationTextureAttachment.isAttached()) {
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, elevationTextureAttachment.texture());
        glUniform1i(elevationTextureUniformHandle, 1);
    }
}

void TexturedPolygon2dTessellatedOpenGl::setIsInverseMasked(bool inversed) { isMaskInversed = inversed; }

void TexturedPolygon2dTessellatedOpenGl::setDebugLabel(const std::string &label) {
    // not used
}

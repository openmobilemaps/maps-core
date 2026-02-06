/*
 * Copyright (c) 2021 Ubique Innovation AG <https://www.ubique.ch>
 *
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this
 *  file, You can obtain one at https://mozilla.org/MPL/2.0/.
 *
 *  SPDX-License-Identifier: MPL-2.0
 */

#include "Polygon2dTessellatedOpenGl.h"
#include "OpenGlHelper.h"
#include <cstring>
#include <cmath>

Polygon2dTessellatedOpenGl::Polygon2dTessellatedOpenGl(const std::shared_ptr<::BaseShaderProgramOpenGl> &shader)
    : shaderProgram(shader) {}

std::shared_ptr<GraphicsObjectInterface> Polygon2dTessellatedOpenGl::asGraphicsObject() { return shared_from_this(); }

std::shared_ptr<MaskingObjectInterface> Polygon2dTessellatedOpenGl::asMaskingObject() { return shared_from_this(); }

bool Polygon2dTessellatedOpenGl::isReady() { return ready; }

void Polygon2dTessellatedOpenGl::setSubdivisionFactor(int32_t factor) {
    std::lock_guard<std::recursive_mutex> lock(dataMutex);
    if (factor != subdivisionFactor) {
        subdivisionFactor = factor;
    }
}

void Polygon2dTessellatedOpenGl::setVertices(const ::SharedBytes & vertices_, const ::SharedBytes & indices_, const ::Vec3D & origin, bool is3d) {
    std::lock_guard<std::recursive_mutex> lock(dataMutex);
    ready = false;
    dataReady = false;

    indices.resize(indices_.elementCount);
    vertices.resize(vertices_.elementCount);
    polygonOrigin = origin;

    this->is3d = is3d;

    if (indices_.elementCount > 0) {
        std::memcpy(indices.data(), (void *)indices_.address, indices_.elementCount * indices_.bytesPerElement);
    }

    if (vertices_.elementCount > 0) {
        std::memcpy(vertices.data(), (void *)vertices_.address,
                    vertices_.elementCount * vertices_.bytesPerElement);
    }

    dataReady = true;
}

void Polygon2dTessellatedOpenGl::setup(const std::shared_ptr<::RenderingContextInterface> &context) {
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

void Polygon2dTessellatedOpenGl::prepareGlData(int program) {
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

    if (!glDataBuffersGenerated) {
        glGenBuffers(1, &indexBuffer);
    }
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuffer);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLushort) * indices.size(), &indices[0], GL_STATIC_DRAW);

    glBindVertexArray(0);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    mMatrixHandle = glGetUniformLocation(program, "umMatrix");
    originOffsetHandle = glGetUniformLocation(program, "uOriginOffset");
    subdivisionFactorHandle = glGetUniformLocation(program, "uSubdivisionFactor");
    originHandle = glGetUniformLocation(program, "uOrigin");
    is3dHandle = glGetUniformLocation(program, "uIs3d");

    glDataBuffersGenerated = true;
}

void Polygon2dTessellatedOpenGl::clear() {
    std::lock_guard<std::recursive_mutex> lock(dataMutex);
    if (ready) {
        removeGlBuffers();
        ready = false;
    }
}

void Polygon2dTessellatedOpenGl::removeGlBuffers() {
    if (glDataBuffersGenerated) {
        glDeleteBuffers(1, &vertexBuffer);
        glDeleteBuffers(1, &indexBuffer);
        glDeleteVertexArrays(1, &vao);
        glDataBuffersGenerated = false;
    }
}

void Polygon2dTessellatedOpenGl::setIsInverseMasked(bool inversed) { isMaskInversed = inversed; }

void Polygon2dTessellatedOpenGl::render(const std::shared_ptr<::RenderingContextInterface> &context, const RenderPassConfig &renderPass,
                                        int64_t vpMatrix, int64_t mMatrix, const ::Vec3D &origin, bool isMasked,
                                        double screenPixelAsRealMeterFactor, bool isScreenSpaceCoords) {
    std::lock_guard<std::recursive_mutex> lock(dataMutex);
    if (!ready || !shaderProgram->isRenderable()) {
        return;
    }

    std::shared_ptr<OpenGlContext> openGlContext = std::static_pointer_cast<OpenGlContext>(context);

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

    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

    drawPolygon(openGlContext, program, vpMatrix, mMatrix, origin, isScreenSpaceCoords);
}

void Polygon2dTessellatedOpenGl::drawPolygon(const std::shared_ptr<::RenderingContextInterface> &context, int program, int64_t vpMatrix,
                                             int64_t mMatrix, const Vec3D &origin, bool isScreenSpaceCoords) {
    std::lock_guard<std::recursive_mutex> lock(dataMutex);
    // Add program to OpenGL environment
    glUseProgram(program);
    glBindVertexArray(vao);

    shaderProgram->preRender(context, isScreenSpaceCoords);

    if(shaderProgram->usesModelMatrix()) {
        glUniformMatrix4fv(mMatrixHandle, 1, false, (GLfloat *) mMatrix);
    }

    glUniform4f(originOffsetHandle, polygonOrigin.x - origin.x, polygonOrigin.y - origin.y, polygonOrigin.z - origin.z, 0.0);

    glPatchParameteri(GL_PATCH_VERTICES, 3);

    glUniform1i(subdivisionFactorHandle, std::pow(2, subdivisionFactor));

    glUniform4f(originHandle, origin.x, origin.y, origin.z, 0.0);

    glUniform1i(is3dHandle, is3d);

    glDrawElements(GL_PATCHES, (unsigned short)indices.size(), GL_UNSIGNED_SHORT, nullptr);

    glBindVertexArray(0);

    glDisable(GL_BLEND);
}

void Polygon2dTessellatedOpenGl::renderAsMask(const std::shared_ptr<::RenderingContextInterface> &context,
                                              const ::RenderPassConfig &renderPass, int64_t vpMatrix, int64_t mMatrix,
                                              const ::Vec3D &origin, double screenPixelAsRealMeterFactor, bool isScreenSpaceCoords) {
    if (!ready) {
        return;
    }

    std::shared_ptr<OpenGlContext> openGlContext = std::static_pointer_cast<OpenGlContext>(context);

    glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
    drawPolygon(openGlContext, program, vpMatrix, mMatrix, origin, isScreenSpaceCoords);
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
}

void Polygon2dTessellatedOpenGl::setDebugLabel(const std::string &label) {
    // not used
}

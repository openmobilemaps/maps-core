/*
 * Copyright (c) 2021 Ubique Innovation AG <https://www.ubique.ch>
 *
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this
 *  file, You can obtain one at https://mozilla.org/MPL/2.0/.
 *
 *  SPDX-License-Identifier: MPL-2.0
 */

#include "Polygon2dOpenGl.h"
#include "OpenGlHelper.h"
#include <cstring>

Polygon2dOpenGl::Polygon2dOpenGl(const std::shared_ptr<::BaseShaderProgramOpenGl> &shader)
    : shaderProgram(shader) {}

std::shared_ptr<GraphicsObjectInterface> Polygon2dOpenGl::asGraphicsObject() { return shared_from_this(); }

std::shared_ptr<MaskingObjectInterface> Polygon2dOpenGl::asMaskingObject() { return shared_from_this(); }

bool Polygon2dOpenGl::isReady() { return ready; }

void Polygon2dOpenGl::setVertices(const ::SharedBytes & vertices_, const ::SharedBytes & indices_, const ::Vec3D & origin) {
    std::lock_guard<std::recursive_mutex> lock(dataMutex);
    ready = false;
    dataReady = false;

    indices.resize(indices_.elementCount);
    vertices.resize(vertices_.elementCount);
    polygonOrigin = origin;

    if(indices_.elementCount > 0) {
        std::memcpy(indices.data(), (void *)indices_.address, indices_.elementCount * indices_.bytesPerElement);
    }

    if(vertices_.elementCount > 0) {
        std::memcpy(vertices.data(), (void *)vertices_.address,
                    vertices_.elementCount * vertices_.bytesPerElement);
    }

    dataReady = true;
}

void Polygon2dOpenGl::setup(const std::shared_ptr<::RenderingContextInterface> &context) {
    std::lock_guard<std::recursive_mutex> lock(dataMutex);
    if (ready || !dataReady)
        return;

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

void Polygon2dOpenGl::prepareGlData(int program) {
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

    glEnableVertexAttribArray(positionHandle);
    glVertexAttribPointer(positionHandle, 3, GL_FLOAT, false, 0, nullptr);

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

    glDataBuffersGenerated = true;
}

void Polygon2dOpenGl::clear() {
    std::lock_guard<std::recursive_mutex> lock(dataMutex);
    if (ready) {
        removeGlBuffers();
        ready = false;
    }
}

void Polygon2dOpenGl::removeGlBuffers() {
    if (glDataBuffersGenerated) {
        glDeleteBuffers(1, &vertexBuffer);
        glDeleteBuffers(1, &indexBuffer);
        glDeleteVertexArrays(1, &vao);
        glDataBuffersGenerated = false;
    }
}

void Polygon2dOpenGl::setIsInverseMasked(bool inversed) { isMaskInversed = inversed; }

void Polygon2dOpenGl::render(const std::shared_ptr<::RenderingContextInterface> &context, const RenderPassConfig &renderPass,
                             int64_t vpMatrix, int64_t mMatrix, const ::Vec3D &origin, bool isMasked,
                             double screenPixelAsRealMeterFactor) {
    std::lock_guard<std::recursive_mutex> lock(dataMutex);
    if (!ready || !shaderProgram->isRenderable())
        return;

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

    drawPolygon(openGlContext, program, vpMatrix, mMatrix, origin);
}

void Polygon2dOpenGl::drawPolygon(const std::shared_ptr<::RenderingContextInterface> &context, int program, int64_t vpMatrix, int64_t mMatrix, const Vec3D &origin) {
    std::lock_guard<std::recursive_mutex> lock(dataMutex);
    // Add program to OpenGL environment
    glUseProgram(program);
    glBindVertexArray(vao);

    shaderProgram->preRender(context);

    if(shaderProgram->usesModelMatrix()) {
        glUniformMatrix4fv(mMatrixHandle, 1, false, (GLfloat *) mMatrix);
    }

    glUniform4f(originOffsetHandle, polygonOrigin.x - origin.x, polygonOrigin.y - origin.y, polygonOrigin.z - origin.z, 0.0);

    // Draw the triangle
    glDrawElements(GL_TRIANGLES, (unsigned short)indices.size(), GL_UNSIGNED_SHORT, nullptr);

    glBindVertexArray(0);

    glDisable(GL_BLEND);
}

void Polygon2dOpenGl::renderAsMask(const std::shared_ptr<::RenderingContextInterface> &context,
                                   const ::RenderPassConfig &renderPass, int64_t vpMatrix, int64_t mMatrix,
                                   const ::Vec3D &origin, double screenPixelAsRealMeterFactor) {
    if (!ready)
        return;

    std::shared_ptr<OpenGlContext> openGlContext = std::static_pointer_cast<OpenGlContext>(context);

    glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
    drawPolygon(openGlContext, program, vpMatrix, mMatrix, origin);
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
}

void Polygon2dOpenGl::setDebugLabel(const std::string &label) {
    // not used
}

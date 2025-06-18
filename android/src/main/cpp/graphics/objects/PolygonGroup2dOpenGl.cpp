/*
 * Copyright (c) 2021 Ubique Innovation AG <https://www.ubique.ch>
 *
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this
 *  file, You can obtain one at https://mozilla.org/MPL/2.0/.
 *
 *  SPDX-License-Identifier: MPL-2.0
 */

#include "PolygonGroup2dOpenGl.h"
#include "RenderVerticesDescription.h"
#include <cmath>
#include <cstring>

PolygonGroup2dOpenGl::PolygonGroup2dOpenGl(const std::shared_ptr<::BaseShaderProgramOpenGl> &shader)
    : shaderProgram(shader) {}

std::shared_ptr<GraphicsObjectInterface> PolygonGroup2dOpenGl::asGraphicsObject() { return shared_from_this(); }

bool PolygonGroup2dOpenGl::isReady() { return ready; }

void
PolygonGroup2dOpenGl::setVertices(const ::SharedBytes & vertices, const ::SharedBytes & indices, const ::Vec3D & origin) {
    std::lock_guard<std::recursive_mutex> lock(dataMutex);
    ready = false;
    dataReady = false;

    polygonIndices.resize(indices.elementCount);
    polygonAttributes.resize(vertices.elementCount);
    polygonOrigin = origin;
    if(indices.elementCount > 0) {
        std::memcpy(polygonIndices.data(), (void *) indices.address,indices.elementCount * indices.bytesPerElement);
    }

    if(vertices.elementCount > 0) {
        std::memcpy(polygonAttributes.data(), (void *) vertices.address,
                    vertices.elementCount * vertices.bytesPerElement);
    }

    dataReady = true;
}

void PolygonGroup2dOpenGl::setup(const std::shared_ptr<::RenderingContextInterface> &context) {
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

    glUseProgram(program);

    if (!glDataBuffersGenerated) {
        glGenVertexArrays(1, &vao);
    }
    glBindVertexArray(vao);

    positionHandle = glGetAttribLocation(program, "vPosition");
    styleIndexHandle = glGetAttribLocation(program, "vStyleIndex");

    if (!glDataBuffersGenerated) {
        glGenBuffers(1, &attribBuffer);
    }
    glBindBuffer(GL_ARRAY_BUFFER, attribBuffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * polygonAttributes.size(), &polygonAttributes[0], GL_STATIC_DRAW);

    size_t floatSize = sizeof(GLfloat);
    size_t stride = 4 * floatSize;
    glEnableVertexAttribArray(positionHandle);
    glVertexAttribPointer(positionHandle, 3, GL_FLOAT, false, stride, nullptr);
    glEnableVertexAttribArray(styleIndexHandle);
    glVertexAttribPointer(styleIndexHandle, 1, GL_FLOAT, false, stride, (float *)(3 * floatSize));

    glBindBuffer(GL_ARRAY_BUFFER, 0);

    if (!glDataBuffersGenerated) {
        glGenBuffers(1, &indexBuffer);
    }
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuffer);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLushort) * polygonIndices.size(), &polygonIndices[0], GL_STATIC_DRAW);

    glBindVertexArray(0);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    vpMatrixHandle = glGetUniformLocation(program, "uvpMatrix");
    mMatrixHandle = glGetUniformLocation(program, "umMatrix");
    originOffsetHandle = glGetUniformLocation(program, "uOriginOffset");
    scaleFactorHandle = glGetUniformLocation(program, "scaleFactors");

    if (const auto &glShader = std::static_pointer_cast<BaseShaderProgramOpenGl>(shaderProgram)) {
        glShader->setupGlObjects(openGlContext);
    }

    ready = true;
    glDataBuffersGenerated = true;
}

void PolygonGroup2dOpenGl::clear() {
    std::lock_guard<std::recursive_mutex> lock(dataMutex);
    if (ready) {
        removeGlBuffers();
        if (const auto &glShader = std::static_pointer_cast<BaseShaderProgramOpenGl>(shaderProgram)) {
            glShader->clearGlObjects();
        }
        ready = false;
    }
}

void PolygonGroup2dOpenGl::removeGlBuffers() {
    if (glDataBuffersGenerated) {
        glDeleteBuffers(1, &attribBuffer);
        glDeleteBuffers(1, &indexBuffer);
        glDeleteVertexArrays(1, &vao);
        glDataBuffersGenerated = false;
    }
}

void PolygonGroup2dOpenGl::setIsInverseMasked(bool inversed) { isMaskInversed = inversed; }

void PolygonGroup2dOpenGl::render(const std::shared_ptr<::RenderingContextInterface> &context, const RenderPassConfig &renderPass,
                                  int64_t vpMatrix, int64_t mMatrix, const ::Vec3D &origin,
                                  bool isMasked, double screenPixelAsRealMeterFactor) {
    std::lock_guard<std::recursive_mutex> lock(dataMutex);
    if (!ready || !shaderProgram->isRenderable())
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

    std::shared_ptr<OpenGlContext> openGlContext = std::static_pointer_cast<OpenGlContext>(context);
    glUseProgram(program);
    glBindVertexArray(vao);

    glUniformMatrix4fv(vpMatrixHandle, 1, false, (GLfloat *)vpMatrix);
    glUniformMatrix4fv(mMatrixHandle, 1, false, (GLfloat *)mMatrix);

    glUniform4f(originOffsetHandle, polygonOrigin.x - origin.x, polygonOrigin.y - origin.y, polygonOrigin.z - origin.z, 0.0);

    if (scaleFactorHandle >= 0) {
        glUniform2f(scaleFactorHandle, screenPixelAsRealMeterFactor,
                    pow(2.0, ceil(log2(screenPixelAsRealMeterFactor))));
    }

    shaderProgram->preRender(context);

    glDrawElements(GL_TRIANGLES, polygonIndices.size(), GL_UNSIGNED_SHORT, nullptr);

    glBindVertexArray(0);

    glDisable(GL_BLEND);
}

void PolygonGroup2dOpenGl::setDebugLabel(const std::string &label) {
    // not used
}

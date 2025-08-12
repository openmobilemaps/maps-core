/*
 * Copyright (c) 2021 Ubique Innovation AG <https://www.ubique.ch>
 *
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this
 *  file, You can obtain one at https://mozilla.org/MPL/2.0/.
 *
 *  SPDX-License-Identifier: MPL-2.0
 */

#include "LineGroup2dOpenGl.h"
#include <cmath>
#include <cstring>
#include <string>

LineGroup2dOpenGl::LineGroup2dOpenGl(const std::shared_ptr<BaseShaderProgramOpenGl> &shader)
    : shaderProgram(shader) {}

std::shared_ptr<GraphicsObjectInterface> LineGroup2dOpenGl::asGraphicsObject() { return shared_from_this(); }

bool LineGroup2dOpenGl::isReady() { return ready; }


void LineGroup2dOpenGl::setLines(const ::SharedBytes & lines, const ::SharedBytes & indices, const Vec3D &origin, bool is3d) {
    std::lock_guard<std::recursive_mutex> lock(dataMutex);
    ready = false;
    dataReady = false;

    lineIndices.resize(indices.elementCount);
    lineAttributes.resize(lines.elementCount);
    lineOrigin = origin;
    this->is3d = is3d;

    if (indices.elementCount > 0) {
        std::memcpy(lineIndices.data(), (void *) indices.address, indices.elementCount * indices.bytesPerElement);
    }
    if (lines.elementCount > 0) {
        std::memcpy(lineAttributes.data(), (void *) lines.address, lines.elementCount * lines.bytesPerElement);
    }

    dataReady = true;
}

void LineGroup2dOpenGl::setup(const std::shared_ptr<::RenderingContextInterface> &context) {
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

    glUseProgram(program);

    if (!glDataBuffersGenerated) {
        glGenVertexArrays(1, &vao);
    }
    glBindVertexArray(vao);

    positionHandle = glGetAttribLocation(program, "position");
    extrudeHandle = glGetAttribLocation(program, "extrude");
    lineSideHandle = glGetAttribLocation(program, "lineSide");
    lengthPrefixHandle = glGetAttribLocation(program, "lengthPrefix");
    lengthCorrectionHandle = glGetAttribLocation(program, "lengthCorrection");
    stylingIndexHandle = glGetAttribLocation(program, "stylingIndex");

    if (!glDataBuffersGenerated) {
        glGenBuffers(1, &vertexAttribBuffer);
    }
    glBindBuffer(GL_ARRAY_BUFFER, vertexAttribBuffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * lineAttributes.size(), &lineAttributes[0], GL_STATIC_DRAW);

    size_t floatSize = sizeof(GLfloat);
    size_t dimensionality = is3d ? 3 : 2;
    size_t sizeAttribGroup = floatSize * dimensionality;
    size_t stride = sizeAttribGroup * 2 + 4 * floatSize;

    glEnableVertexAttribArray(positionHandle);
    glVertexAttribPointer(positionHandle, dimensionality, GL_FLOAT, false, stride, nullptr);

    glEnableVertexAttribArray(extrudeHandle);
    glVertexAttribPointer(extrudeHandle, dimensionality, GL_FLOAT, false, stride, (float *)(sizeAttribGroup * 1));

    if (lineSideHandle >= 0) {
        glEnableVertexAttribArray(lineSideHandle);
        glVertexAttribPointer(lineSideHandle, 1, GL_FLOAT, false, stride, (float *) (sizeAttribGroup * 2));
    }

    if (lengthPrefixHandle >= 0) {
        glEnableVertexAttribArray(lengthPrefixHandle);
        glVertexAttribPointer(lengthPrefixHandle, 1, GL_FLOAT, false, stride, (float *) (sizeAttribGroup * 2 + 1 * floatSize));
    }

    if (lengthCorrectionHandle >= 0) {
        glEnableVertexAttribArray(lengthCorrectionHandle);
        glVertexAttribPointer(lengthCorrectionHandle, 1, GL_FLOAT, false, stride, (float *) (sizeAttribGroup * 2 + 2 * floatSize));
    }

    glEnableVertexAttribArray(stylingIndexHandle);
    glVertexAttribPointer(stylingIndexHandle, 1, GL_FLOAT, false, stride, (float *)(sizeAttribGroup * 2 + 3 * floatSize));

    glBindBuffer(GL_ARRAY_BUFFER, 0);

    if (!glDataBuffersGenerated) {
        glGenBuffers(1, &indexBuffer);
    }
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuffer);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLushort) * lineIndices.size(), &lineIndices[0], GL_STATIC_DRAW);

    glBindVertexArray(0);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    mMatrixHandle = glGetUniformLocation(program, "umMatrix");
    originOffsetHandle = glGetUniformLocation(program, "originOffset");
    lineOriginHandle = glGetUniformLocation(program, "uLineOrigin");
    scaleFactorHandle = glGetUniformLocation(program, "scalingFactor");

    shaderProgram->setupGlObjects(openGlContext);

    ready = true;
    glDataBuffersGenerated = true;
}

void LineGroup2dOpenGl::clear() {
    std::lock_guard<std::recursive_mutex> lock(dataMutex);
    if (ready) {
        removeGlBuffers();
        shaderProgram->clearGlObjects();
        ready = false;
    }
}

void LineGroup2dOpenGl::removeGlBuffers() {
    if (glDataBuffersGenerated) {
        glDeleteBuffers(1, &vertexAttribBuffer);
        glDeleteBuffers(1, &indexBuffer);
        glDeleteVertexArrays(1, &vao);
        glDataBuffersGenerated = false;
    }
}

void LineGroup2dOpenGl::setIsInverseMasked(bool inversed) { isMaskInversed = inversed; }

void LineGroup2dOpenGl::render(const std::shared_ptr<::RenderingContextInterface> &context, const RenderPassConfig &renderPass,
                               int64_t vpMatrix, int64_t mMatrix, const ::Vec3D &origin, bool isMasked,
                               double screenPixelAsRealMeterFactor) {
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

    // Add program to OpenGL environment
    glUseProgram(program);

    glBindVertexArray(vao);

    if(shaderProgram->usesModelMatrix()) {
        glUniformMatrix4fv(mMatrixHandle, 1, false, (GLfloat *)mMatrix);
    }

    glUniform4f(originOffsetHandle, lineOrigin.x - origin.x, lineOrigin.y - origin.y, lineOrigin.z - origin.z, 0.0);

    shaderProgram->preRender(openGlContext);

    // Draw the triangle
    glDrawElements(GL_TRIANGLES, lineIndices.size(), GL_UNSIGNED_SHORT, nullptr);

    glBindVertexArray(0);

    glDisable(GL_BLEND);
}

void LineGroup2dOpenGl::setDebugLabel(const std::string &label) {
    // not used
}

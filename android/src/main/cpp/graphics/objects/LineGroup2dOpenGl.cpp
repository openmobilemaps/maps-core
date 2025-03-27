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
#include "BaseShaderProgramOpenGl.h"
#include <cmath>
#include <cstring>
#include <string>

LineGroup2dOpenGl::LineGroup2dOpenGl(const std::shared_ptr<::ShaderProgramInterface> &shader)
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

    pointAHandle = glGetAttribLocation(program, "vPointA");
    pointBHandle = glGetAttribLocation(program, "vPointB");
    vertexIndexHandle = glGetAttribLocation(program, "vVertexIndex");
    segmentStartLPosHandle = glGetAttribLocation(program, "vSegmentStartLPos");
    styleInfoHandle = glGetAttribLocation(program, "vStyleInfo");

    if (!glDataBuffersGenerated) {
        glGenBuffers(1, &vertexAttribBuffer);
    }
    glBindBuffer(GL_ARRAY_BUFFER, vertexAttribBuffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * lineAttributes.size(), &lineAttributes[0], GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    if (!glDataBuffersGenerated) {
        glGenBuffers(1, &indexBuffer);
    }
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuffer);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLuint) * lineIndices.size(), &lineIndices[0], GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    vpMatrixHandle = glGetUniformLocation(program, "uvpMatrix");
    mMatrixHandle = glGetUniformLocation(program, "umMatrix");
    originOffsetHandle = glGetUniformLocation(program, "uOriginOffset");
    lineOriginHandle = glGetUniformLocation(program, "uLineOrigin");
    scaleFactorHandle = glGetUniformLocation(program, "scaleFactor");

    if (const auto &glShader = std::static_pointer_cast<BaseShaderProgramOpenGl>(shaderProgram)) {
        glShader->setupGlObjects(openGlContext);
    }

    ready = true;
    glDataBuffersGenerated = true;
}

void LineGroup2dOpenGl::clear() {
    std::lock_guard<std::recursive_mutex> lock(dataMutex);
    if (ready) {
        removeGlBuffers();
        if (const auto &glShader = std::static_pointer_cast<BaseShaderProgramOpenGl>(shaderProgram)) {
            glShader->clearGlObjects();
        }
        ready = false;
    }
}

void LineGroup2dOpenGl::removeGlBuffers() {
    if (glDataBuffersGenerated) {
        glDeleteBuffers(1, &vertexAttribBuffer);
        glDeleteBuffers(1, &indexBuffer);
        glDataBuffersGenerated = false;
    }
}

void LineGroup2dOpenGl::setIsInverseMasked(bool inversed) { isMaskInversed = inversed; }

void LineGroup2dOpenGl::render(const std::shared_ptr<::RenderingContextInterface> &context, const RenderPassConfig &renderPass,
                               int64_t vpMatrix, int64_t mMatrix, const ::Vec3D &origin, bool isMasked,
                               double screenPixelAsRealMeterFactor) {

    if (!ready) {
        return;
    }

    std::shared_ptr<OpenGlContext> openGlContext = std::static_pointer_cast<OpenGlContext>(context);
    if (isMasked) {
        glStencilFunc(GL_EQUAL, isMaskInversed ? 0 : 128, 255);
    } else {
        glEnable(GL_STENCIL_TEST);
        glStencilMask(0xFF);
        glClearStencil(0x0);
        glClear(GL_STENCIL_BUFFER_BIT);
        glStencilFunc(GL_NOTEQUAL, 0x1, 0xFF);
    }
    glStencilOp(GL_KEEP, GL_KEEP, GL_INCR);
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

    // Add program to OpenGL environment
    glUseProgram(program);

    // Apply the projection and view transformation
    glUniformMatrix4fv(vpMatrixHandle, 1, false, (GLfloat *)vpMatrix);
    glUniformMatrix4fv(mMatrixHandle, 1, false, (GLfloat *)mMatrix);
    glUniform4f(originOffsetHandle, lineOrigin.x - origin.x, lineOrigin.y - origin.y, lineOrigin.z - origin.z, 0.0);
    if (lineOriginHandle >= 0) {
        glUniform4f(lineOriginHandle, lineOrigin.x, lineOrigin.y, lineOrigin.z, 0.0);
    }
    glUniform1f(scaleFactorHandle, screenPixelAsRealMeterFactor);

    shaderProgram->preRender(openGlContext);

    // Prepare the vertex attributes
    size_t floatSize = sizeof(GLfloat);
    size_t dimensionality = is3d ? 3 : 2;
    size_t sizeAttribGroup = floatSize * dimensionality;
    size_t stride = sizeAttribGroup * 2 + 3 * floatSize;
    glBindBuffer(GL_ARRAY_BUFFER, vertexAttribBuffer);
    glEnableVertexAttribArray(pointAHandle);
    glVertexAttribPointer(pointAHandle, dimensionality, GL_FLOAT, false, stride, nullptr);
    glEnableVertexAttribArray(pointBHandle);
    glVertexAttribPointer(pointBHandle, dimensionality, GL_FLOAT, false, stride, (float *)(sizeAttribGroup * 1));
    glEnableVertexAttribArray(vertexIndexHandle);
    glVertexAttribPointer(vertexIndexHandle, 1, GL_FLOAT, false, stride, (float *)(sizeAttribGroup * 2));
    if (segmentStartLPosHandle >= 0) {
        glEnableVertexAttribArray(segmentStartLPosHandle);
        glVertexAttribPointer(segmentStartLPosHandle, 1, GL_FLOAT, false, stride, (float *) (sizeAttribGroup * 2 + floatSize));
    }
    glEnableVertexAttribArray(styleInfoHandle);
    glVertexAttribPointer(styleInfoHandle, 1, GL_FLOAT, false, stride, (float *)(sizeAttribGroup * 2 + 2 * floatSize));
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    // Draw the triangle
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuffer);
    glDrawElements(GL_TRIANGLES, lineIndices.size(), GL_UNSIGNED_INT, nullptr);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    // Disable vertex array
    glDisableVertexAttribArray(pointAHandle);
    glDisableVertexAttribArray(pointBHandle);
    glDisableVertexAttribArray(vertexIndexHandle);
    if (segmentStartLPosHandle >= 0) {
        glDisableVertexAttribArray(segmentStartLPosHandle);
    }
    glDisableVertexAttribArray(styleInfoHandle);

    glDisable(GL_BLEND);
    if (!isMasked) {
        glDisable(GL_STENCIL_TEST);
    }
}

void LineGroup2dOpenGl::setDebugLabel(const std::string &label) {
    // not used
}

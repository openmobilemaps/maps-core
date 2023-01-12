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

Polygon2dOpenGl::Polygon2dOpenGl(const std::shared_ptr<::ShaderProgramInterface> &shader)
    : shaderProgram(shader) {}

std::shared_ptr<GraphicsObjectInterface> Polygon2dOpenGl::asGraphicsObject() { return shared_from_this(); }

std::shared_ptr<MaskingObjectInterface> Polygon2dOpenGl::asMaskingObject() { return shared_from_this(); }

bool Polygon2dOpenGl::isReady() { return ready; }

void Polygon2dOpenGl::setVertices(const ::SharedBytes & vertices_, const ::SharedBytes & indices_) {
    std::lock_guard<std::recursive_mutex> lock(dataMutex);
    ready = false;
    dataReady = false;

    indices.resize(indices_.elementCount);
    vertices.resize(vertices_.elementCount);

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
    if (openGlContext->getProgram(shaderProgram->getProgramName()) == 0) {
        shaderProgram->setupProgram(openGlContext);
    }
    programHandle = openGlContext->getProgram(shaderProgram->getProgramName());

    prepareGlData(openGlContext);
    ready = true;
}

void Polygon2dOpenGl::prepareGlData(const std::shared_ptr<OpenGlContext> &openGlContext) {
    glUseProgram(programHandle);

    positionHandle = glGetAttribLocation(programHandle, "vPosition");
    glGenBuffers(1, &vertexBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * vertices.size(), &vertices[0], GL_STATIC_DRAW);

    glBindBuffer(GL_ARRAY_BUFFER, 0);

    glGenBuffers(1, &indexBuffer);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuffer);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLushort) * indices.size(), &indices[0], GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    mvpMatrixHandle = glGetUniformLocation(programHandle, "uMVPMatrix");
}

void Polygon2dOpenGl::clear() {
    std::lock_guard<std::recursive_mutex> lock(dataMutex);
    if (ready) {
        removeGlBuffers();
        ready = false;
    }
}

void Polygon2dOpenGl::removeGlBuffers() {
    glDeleteBuffers(1, &vertexBuffer);
    glDeleteBuffers(1, &indexBuffer);
}

void Polygon2dOpenGl::setIsInverseMasked(bool inversed) { isMaskInversed = inversed; }

void Polygon2dOpenGl::render(const std::shared_ptr<::RenderingContextInterface> &context, const RenderPassConfig &renderPass,
                             int64_t mvpMatrix, bool isMasked, double screenPixelAsRealMeterFactor) {
    if (!ready)
        return;

    std::shared_ptr<OpenGlContext> openGlContext = std::static_pointer_cast<OpenGlContext>(context);

    if (isMasked) {
        if (isMaskInversed) {
            glStencilFunc(GL_EQUAL, 0, 255);
        } else {
            glStencilFunc(GL_EQUAL, 128, 255);
        }
    }
    glStencilOp(GL_KEEP, GL_KEEP, GL_INCR);
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

    drawPolygon(openGlContext, programHandle, mvpMatrix);
}

void Polygon2dOpenGl::drawPolygon(std::shared_ptr<OpenGlContext> openGlContext, int program, int64_t mvpMatrix) {
    // Add program to OpenGL environment
    glUseProgram(program);

    shaderProgram->preRender(openGlContext);

    glEnableVertexAttribArray(positionHandle);
    glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
    glVertexAttribPointer(positionHandle, 3, GL_FLOAT, false, 0, nullptr);

    glBindBuffer(GL_ARRAY_BUFFER, 0);

    // Apply the projection and view transformation
    glUniformMatrix4fv(mvpMatrixHandle, 1, false, (GLfloat *)mvpMatrix);

    // Enable blending
    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

    // Draw the triangle
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuffer);
    glDrawElements(GL_TRIANGLES, (unsigned short)indices.size(), GL_UNSIGNED_SHORT, nullptr);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    // Disable vertex array
    glDisableVertexAttribArray(positionHandle);

    glDisable(GL_BLEND);
}

void Polygon2dOpenGl::renderAsMask(const std::shared_ptr<::RenderingContextInterface> &context,
                                   const ::RenderPassConfig &renderPass, int64_t mvpMatrix, double screenPixelAsRealMeterFactor) {
    if (!ready)
        return;

    std::shared_ptr<OpenGlContext> openGlContext = std::static_pointer_cast<OpenGlContext>(context);

    glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
    drawPolygon(openGlContext, programHandle, mvpMatrix);
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
}

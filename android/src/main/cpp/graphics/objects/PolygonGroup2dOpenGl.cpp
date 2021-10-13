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

PolygonGroup2dOpenGl::PolygonGroup2dOpenGl(const std::shared_ptr<::ShaderProgramInterface> &shader) : shaderProgram(shader) {}

std::shared_ptr<GraphicsObjectInterface> PolygonGroup2dOpenGl::asGraphicsObject() {
    return shared_from_this();
}

bool PolygonGroup2dOpenGl::isReady() {
    return ready;
}

void
PolygonGroup2dOpenGl::setVertices(const std::vector<RenderVerticesDescription> &vertices, const std::vector<int32_t> &indices) {
    ready = false;
    dataReady = false;

    polygonIndices.clear();
    polygonAttributes.clear();

    int numPolygons = vertices.size();
    for (int polygonIndex = 0; polygonIndex < numPolygons; polygonIndex++) {
        int styleIndex = vertices[polygonIndex].styleIndex;
        int numVertices = (int) vertices[polygonIndex].vertices.size();

        for (int i = 0; i < numVertices; i++) {
            const Vec2D &p = vertices[polygonIndex].vertices[i];
            // Position
            polygonAttributes.push_back(p.x);
            polygonAttributes.push_back(p.y);
            // StyleIndex
            polygonAttributes.push_back(styleIndex);
        }

    }
    // Indices
    int numIndices = indices.size();
    for (int i = 0; i < numIndices; i++) {
        polygonIndices.push_back(indices[i]);
    }

    dataReady = true;
}

void PolygonGroup2dOpenGl::setup(const std::shared_ptr<::RenderingContextInterface> &context) {
    if (ready || !dataReady) return;

    std::shared_ptr<OpenGlContext> openGlContext = std::static_pointer_cast<OpenGlContext>(context);
    if (openGlContext->getProgram(shaderProgram->getProgramName()) == 0) {
        shaderProgram->setupProgram(openGlContext);
    }

    int program = openGlContext->getProgram(shaderProgram->getProgramName());
    glUseProgram(program);

    positionHandle = glGetAttribLocation(program, "vPosition");
    styleIndexHandle = glGetAttribLocation(program, "vStyleIndex");

    glGenBuffers(1, &attribBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, attribBuffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * polygonAttributes.size(), &polygonAttributes[0], GL_STATIC_DRAW);
    OpenGlHelper::checkGlError("Setup attribute buffer");
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    glGenBuffers(1, &indexBuffer);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuffer);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLushort) * polygonIndices.size(), &polygonIndices[0], GL_STATIC_DRAW);
    OpenGlHelper::checkGlError("Setup index buffer");
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    mvpMatrixHandle = glGetUniformLocation(program, "uMVPMatrix");
    OpenGlHelper::checkGlError("glGetUniformLocation uMVPMatrix");

    ready = true;
}

void PolygonGroup2dOpenGl::clear() {
    ready = false;
    glDeleteBuffers(1, &attribBuffer);
    glDeleteBuffers(1, &indexBuffer);
}

void PolygonGroup2dOpenGl::render(const std::shared_ptr<::RenderingContextInterface> &context, const RenderPassConfig &renderPass,
                                  int64_t mvpMatrix, bool isMasked, double screenPixelAsRealMeterFactor) {
    if (!ready) return;

    if (isMasked) {
        glStencilFunc(GL_EQUAL, 128, 128);
        glStencilOp(GL_ZERO, GL_ZERO, GL_ZERO);
    }

    std::shared_ptr<OpenGlContext> openGlContext = std::static_pointer_cast<OpenGlContext>(context);
    int mProgram = openGlContext->getProgram(shaderProgram->getProgramName());
    glUseProgram(mProgram);
    OpenGlHelper::checkGlError("glUseProgram PolygonGroupOpenGl");

    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

    glUniformMatrix4fv(mvpMatrixHandle, 1, false, (GLfloat *)mvpMatrix);
    OpenGlHelper::checkGlError("glUniformMatrix4fv");

    shaderProgram->preRender(context);

    size_t floatSize = sizeof(GLfloat);
    size_t stride = 3 * floatSize;

    glBindBuffer(GL_ARRAY_BUFFER, attribBuffer);
    glEnableVertexAttribArray(positionHandle);
    glVertexAttribPointer(positionHandle, 2, GL_FLOAT, false, stride, nullptr);
    glEnableVertexAttribArray(styleIndexHandle);
    glVertexAttribPointer(styleIndexHandle, 1, GL_FLOAT, false, stride, (float *) (2 * floatSize));
    OpenGlHelper::checkGlError("glEnableVertexAttribArray attribBuffer");
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuffer);
    glDrawElements(GL_TRIANGLES, polygonIndices.size(), GL_UNSIGNED_SHORT, nullptr);

    OpenGlHelper::checkGlError("glDrawElements");

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    // Disable vertex array
    glDisableVertexAttribArray(positionHandle);
    glDisableVertexAttribArray(styleIndexHandle);
}

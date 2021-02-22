/*
 * Copyright (c) 2021 Ubique Innovation AG <https://www.ubique.ch>
 *
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this
 *  file, You can obtain one at https://mozilla.org/MPL/2.0/.
 *
 *  SPDX-License-Identifier: MPL-2.0
 */

#include "Line2dOpenGl.h"
#include <cmath>

Line2dOpenGl::Line2dOpenGl(const std::shared_ptr<::LineShaderProgramInterface> &shader)
    : shaderProgram(shader) {}

std::shared_ptr<GraphicsObjectInterface> Line2dOpenGl::asGraphicsObject() { return shared_from_this(); }

bool Line2dOpenGl::isReady() { return ready; }

void Line2dOpenGl::setLinePositions(const std::vector<::Vec2D> &positions) {
    lineCoordinates = positions;
    ready = false;
}

void Line2dOpenGl::setup(const std::shared_ptr<::RenderingContextInterface> &context) {
    if (ready)
        return;

    std::shared_ptr<OpenGlContext> openGlContext = std::static_pointer_cast<OpenGlContext>(context);
    if (openGlContext->getProgram(shaderProgram->getPointProgramName()) == 0) {
        shaderProgram->setupPointProgram(openGlContext);
    }
    if (openGlContext->getProgram(shaderProgram->getRectProgramName()) == 0) {
        shaderProgram->setupRectProgram(openGlContext);
        ;
    }
    initializeLineAndPoints();
    ready = true;
}

void Line2dOpenGl::initializeLineAndPoints() {
    pointsVertexBuffer = std::vector<GLfloat>();
    for (auto &p : lineCoordinates) {
        pointsVertexBuffer.push_back(p.x);
        pointsVertexBuffer.push_back(p.y);
        pointsVertexBuffer.push_back(0.0);
    }

    pointCount = (int)lineCoordinates.size();

    for (int i = 0; i < (pointCount - 1); i++) {
        int iNext = i + 1;

        lineVertexBuffer.push_back(pointsVertexBuffer[i * 3 + 0]);
        lineVertexBuffer.push_back(pointsVertexBuffer[i * 3 + 1]);
        lineVertexBuffer.push_back(pointsVertexBuffer[i * 3 + 2]);

        lineVertexBuffer.push_back(pointsVertexBuffer[i * 3 + 0]);
        lineVertexBuffer.push_back(pointsVertexBuffer[i * 3 + 1]);
        lineVertexBuffer.push_back(pointsVertexBuffer[i * 3 + 2]);

        float lineNormalX = -(pointsVertexBuffer[iNext * 3 + 1] - pointsVertexBuffer[i * 3 + 1]);
        float lineNormalY = pointsVertexBuffer[iNext * 3 + 0] - pointsVertexBuffer[i * 3 + 0];
        float lineLength = std::sqrt(lineNormalX * lineNormalX + lineNormalY * lineNormalY);

        lineNormalBuffer.push_back(lineNormalX / lineLength);
        lineNormalBuffer.push_back(lineNormalY / lineLength);
        lineNormalBuffer.push_back(0.0);

        lineNormalBuffer.push_back(-lineNormalBuffer[12 * i + 0]);
        lineNormalBuffer.push_back(-lineNormalBuffer[12 * i + 1]);
        lineNormalBuffer.push_back(0.0);

        lineVertexBuffer.push_back(pointsVertexBuffer[iNext * 3 + 0]);
        lineVertexBuffer.push_back(pointsVertexBuffer[iNext * 3 + 1]);
        lineVertexBuffer.push_back(pointsVertexBuffer[iNext * 3 + 2]);

        lineVertexBuffer.push_back(pointsVertexBuffer[iNext * 3 + 0]);
        lineVertexBuffer.push_back(pointsVertexBuffer[iNext * 3 + 1]);
        lineVertexBuffer.push_back(pointsVertexBuffer[iNext * 3 + 2]);

        lineNormalBuffer.push_back(lineNormalX / lineLength);
        lineNormalBuffer.push_back(lineNormalY / lineLength);
        lineNormalBuffer.push_back(0.0);

        lineNormalBuffer.push_back(-lineNormalBuffer[12 * i + 0]);
        lineNormalBuffer.push_back(-lineNormalBuffer[12 * i + 1]);
        lineNormalBuffer.push_back(0.0);

        lineIndexBuffer.push_back(4 * i);
        lineIndexBuffer.push_back(4 * i + 1);
        lineIndexBuffer.push_back(4 * i + 2);

        lineIndexBuffer.push_back(4 * i + 2);
        lineIndexBuffer.push_back(4 * i + 1);
        lineIndexBuffer.push_back(4 * i + 3);
    }
}

void Line2dOpenGl::clear() {
    // TODO TOPO-1470: Clear GL-Data (careful with shared program/textures)
    ready = false;
}

void Line2dOpenGl::render(const std::shared_ptr<::RenderingContextInterface> &context, const RenderPassConfig &renderPass,
                          int64_t mvpMatrix) {
    if (!ready)
        return;

    std::shared_ptr<OpenGlContext> openGlContext = std::static_pointer_cast<OpenGlContext>(context);

    glEnable(GL_STENCIL_TEST);
    glStencilMask(0xFF);
    glClearStencil(0x0);
    glClear(GL_STENCIL_BUFFER_BIT);
    glStencilFunc(GL_NOTEQUAL, 0x1, 0xFF);
    glStencilOp(GL_KEEP, GL_KEEP, GL_INCR);
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

    drawLineSegments(openGlContext, mvpMatrix);

    glStencilFunc(GL_NOTEQUAL, 0x1, 0xFF);
    glStencilOp(GL_KEEP, GL_KEEP, GL_INCR);
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

    // drawPoints(openGlContext, mvpMatrix);

    glDisable(GL_STENCIL_TEST);
}

void Line2dOpenGl::drawLineSegments(std::shared_ptr<OpenGlContext> openGlContext, int64_t mvpMatrix) {
    int program = openGlContext->getProgram(shaderProgram->getRectProgramName());
    // Add program to OpenGL environment
    glUseProgram(program);

    // get handle to vertex shader's vPosition member
    int positionHandle = glGetAttribLocation(program, "vPosition");
    // Enable a handle to the triangle vertices
    glEnableVertexAttribArray(positionHandle);

    // get handle to shape's transformation matrix
    int mMVPMatrixHandle = glGetUniformLocation(program, "uMVPMatrix");
    OpenGlHelper::checkGlError("glGetUniformLocation");

    // Apply the projection and view transformation
    glUniformMatrix4fv(mMVPMatrixHandle, 1, false, (GLfloat *)mvpMatrix);
    OpenGlHelper::checkGlError("glUniformMatrix4fv");

    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

    shaderProgram->preRenderRect(openGlContext);

    // Prepare the triangle coordinate data
    glVertexAttribPointer(positionHandle, 3, GL_FLOAT, false, 12, &lineVertexBuffer[0]);

    // get handle to vertex shader's vPosition member
    int normalHandle = glGetAttribLocation(program, "vNormal");
    // Enable a handle to the triangle vertices
    glEnableVertexAttribArray(normalHandle);

    // Prepare the triangle coordinate data
    glVertexAttribPointer(normalHandle, 3, GL_FLOAT, false, 12, &lineNormalBuffer[0]);

    // Draw the triangle
    glDrawElements(GL_TRIANGLES, lineIndexBuffer.size(), GL_UNSIGNED_INT, &lineIndexBuffer[0]);

    // Disable vertex array
    glDisableVertexAttribArray(positionHandle);

    glDisable(GL_BLEND);
}

void Line2dOpenGl::drawPoints(std::shared_ptr<OpenGlContext> openGlContext, int64_t mvpMatrix) {
    int program = openGlContext->getProgram(shaderProgram->getPointProgramName());
    // Add program to OpenGL environment
    glUseProgram(program);

    // get handle to vertex shader's vPosition member
    int positionHandle = glGetAttribLocation(program, "vPosition");
    // Enable a handle to the triangle vertices
    glEnableVertexAttribArray(positionHandle);

    // get handle to shape's transformation matrix
    int mVPMatrixHandle = glGetUniformLocation(program, "uMVPMatrix");
    OpenGlHelper::checkGlError("glGetUniformLocation");

    // Apply the projection and view transformation
    glUniformMatrix4fv(mVPMatrixHandle, 1, false, (GLfloat *)mvpMatrix);
    OpenGlHelper::checkGlError("glUniformMatrix4fv");

    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

    shaderProgram->preRenderPoint(openGlContext);

    // Prepare the triangle coordinate data
    glVertexAttribPointer(positionHandle, 3, GL_FLOAT, false, 12, &pointsVertexBuffer[0]);

    glDrawArrays(GL_POINTS, 0, pointCount);

    // Disable vertex array
    glDisableVertexAttribArray(positionHandle);

    glDisable(GL_BLEND);
}

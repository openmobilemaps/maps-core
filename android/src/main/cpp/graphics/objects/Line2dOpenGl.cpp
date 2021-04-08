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

Line2dOpenGl::Line2dOpenGl(const std::shared_ptr<::ShaderProgramInterface> &shader)
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
    if (openGlContext->getProgram(shaderProgram->getProgramName()) == 0) {
        shaderProgram->setupProgram(openGlContext);
    }
    initializeLineAndPoints();
    ready = true;
}

void Line2dOpenGl::initializeLineAndPoints() {
    pointCount = (int)lineCoordinates.size();
    for (int i = 0; i < (pointCount - 1); i++) {
        const Vec2D &p = lineCoordinates[i];
        const Vec2D &pNext = lineCoordinates[i + 1];

        float lengthNormalX = pNext.x - p.x;
        float lengthNormalY = pNext.y - p.y;
        float lineLength = std::sqrt(lengthNormalX * lengthNormalX + lengthNormalY * lengthNormalY);
        lengthNormalX = lengthNormalX / lineLength;
        lengthNormalY = lengthNormalY / lineLength;
        float widthNormalX = -lengthNormalY;
        float widthNormalY = lengthNormalX;

        // Vertex 1
        lineVertexBuffer.push_back(p.x);
        lineVertexBuffer.push_back(p.y);
        lineVertexBuffer.push_back(0.0);

        lineWidthNormalBuffer.push_back(-widthNormalX);
        lineWidthNormalBuffer.push_back(-widthNormalY);
        lineWidthNormalBuffer.push_back(0.0);

        lineLengthNormalBuffer.push_back(-lengthNormalX);
        lineLengthNormalBuffer.push_back(-lengthNormalY);
        lineLengthNormalBuffer.push_back(0.0);

        linePointABuffer.push_back(p.x);
        linePointABuffer.push_back(p.y);
        linePointABuffer.push_back(0.0);
        linePointBBuffer.push_back(pNext.x);
        linePointBBuffer.push_back(pNext.y);
        linePointBBuffer.push_back(0.0);

        // Vertex 2
        lineVertexBuffer.push_back(p.x);
        lineVertexBuffer.push_back(p.y);
        lineVertexBuffer.push_back(0.0);

        lineWidthNormalBuffer.push_back(widthNormalX);
        lineWidthNormalBuffer.push_back(widthNormalY);
        lineWidthNormalBuffer.push_back(0.0);

        lineLengthNormalBuffer.push_back(-lengthNormalX);
        lineLengthNormalBuffer.push_back(-lengthNormalY);
        lineLengthNormalBuffer.push_back(0.0);

        linePointABuffer.push_back(p.x);
        linePointABuffer.push_back(p.y);
        linePointABuffer.push_back(0.0);
        linePointBBuffer.push_back(pNext.x);
        linePointBBuffer.push_back(pNext.y);
        linePointBBuffer.push_back(0.0);

        // Vertex 3
        lineVertexBuffer.push_back(pNext.x);
        lineVertexBuffer.push_back(pNext.y);
        lineVertexBuffer.push_back(0.0);

        lineWidthNormalBuffer.push_back(widthNormalX);
        lineWidthNormalBuffer.push_back(widthNormalY);
        lineWidthNormalBuffer.push_back(0.0);

        lineLengthNormalBuffer.push_back(lengthNormalX);
        lineLengthNormalBuffer.push_back(lengthNormalY);
        lineLengthNormalBuffer.push_back(0.0);

        linePointABuffer.push_back(p.x);
        linePointABuffer.push_back(p.y);
        linePointABuffer.push_back(0.0);
        linePointBBuffer.push_back(pNext.x);
        linePointBBuffer.push_back(pNext.y);
        linePointBBuffer.push_back(0.0);

        // Vertex 4
        lineVertexBuffer.push_back(pNext.x);
        lineVertexBuffer.push_back(pNext.y);
        lineVertexBuffer.push_back(0.0);

        lineWidthNormalBuffer.push_back(-widthNormalX);
        lineWidthNormalBuffer.push_back(-widthNormalY);
        lineWidthNormalBuffer.push_back(0.0);

        lineLengthNormalBuffer.push_back(lengthNormalX);
        lineLengthNormalBuffer.push_back(lengthNormalY);
        lineLengthNormalBuffer.push_back(0.0);

        linePointABuffer.push_back(p.x);
        linePointABuffer.push_back(p.y);
        linePointABuffer.push_back(0.0);
        linePointBBuffer.push_back(pNext.x);
        linePointBBuffer.push_back(pNext.y);
        linePointBBuffer.push_back(0.0);

        // Vertex indices
        lineIndexBuffer.push_back(4 * i);
        lineIndexBuffer.push_back(4 * i + 1);
        lineIndexBuffer.push_back(4 * i + 2);

        lineIndexBuffer.push_back(4 * i);
        lineIndexBuffer.push_back(4 * i + 2);
        lineIndexBuffer.push_back(4 * i + 3);
    }
}

void Line2dOpenGl::clear() {
    // TODO TOPO-1470: Clear GL-Data (careful with shared program/textures)
    ready = false;
}

void Line2dOpenGl::render(const std::shared_ptr<::RenderingContextInterface> &context, const RenderPassConfig &renderPass,
                          int64_t mvpMatrix, bool isMasked, double screenPixelAsRealMeterFactor) {
    if (!ready)
        return;

    if (isMasked) {
        glStencilFunc(GL_EQUAL, 128, 128);
        glStencilOp(GL_ZERO, GL_ZERO, GL_ZERO);
    }

    std::shared_ptr<OpenGlContext> openGlContext = std::static_pointer_cast<OpenGlContext>(context);

    glEnable(GL_STENCIL_TEST);
    glStencilMask(0xFF);
    glClearStencil(0x0);
    glClear(GL_STENCIL_BUFFER_BIT);
    glStencilFunc(GL_NOTEQUAL, 0x1, 0xFF);
    glStencilOp(GL_KEEP, GL_KEEP, GL_INCR);
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

    drawLineSegments(openGlContext, mvpMatrix, screenPixelAsRealMeterFactor);

    glDisable(GL_STENCIL_TEST);
}

void Line2dOpenGl::drawLineSegments(std::shared_ptr<OpenGlContext> openGlContext, int64_t mvpMatrix, float widthScaleFactor) {
    int program = openGlContext->getProgram(shaderProgram->getProgramName());
    // Add program to OpenGL environment
    glUseProgram(program);

    // get handle to vertex shader's vPosition member
    int positionHandle = glGetAttribLocation(program, "vPosition");
    // Enable a handle to the triangle vertices
    glEnableVertexAttribArray(positionHandle);

    int widthNormalHandle = glGetAttribLocation(program, "vWidthNormal");
    glEnableVertexAttribArray(widthNormalHandle);
    int lengthNormalHandle = glGetAttribLocation(program, "vLengthNormal");
    glEnableVertexAttribArray(lengthNormalHandle);

    int pointAHandle = glGetAttribLocation(program, "vPointA");
    glEnableVertexAttribArray(pointAHandle);
    int pointBHandle = glGetAttribLocation(program, "vPointB");
    glEnableVertexAttribArray(pointBHandle);

    // get handle to shape's transformation matrix
    int mMVPMatrixHandle = glGetUniformLocation(program, "uMVPMatrix");
    int scaleFactorHandle = glGetUniformLocation(program, "scaleFactor");
    OpenGlHelper::checkGlError("glGetUniformLocation");

    // Apply the projection and view transformation
    glUniformMatrix4fv(mMVPMatrixHandle, 1, false, (GLfloat *)mvpMatrix);
    glUniform1f(scaleFactorHandle, widthScaleFactor);
    OpenGlHelper::checkGlError("glUniformMatrix4fv and glUniformM1f");

    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

    shaderProgram->preRender(openGlContext);

    // Prepare the triangle coordinate data
    glVertexAttribPointer(positionHandle, 3, GL_FLOAT, false, 12, &lineVertexBuffer[0]);

    glVertexAttribPointer(widthNormalHandle, 3, GL_FLOAT, false, 12, &lineWidthNormalBuffer[0]);
    glVertexAttribPointer(lengthNormalHandle, 3, GL_FLOAT, false, 12, &lineLengthNormalBuffer[0]);

    glVertexAttribPointer(pointAHandle, 3, GL_FLOAT, false, 12, &linePointABuffer[0]);
    glVertexAttribPointer(pointBHandle, 3, GL_FLOAT, false, 12, &linePointBBuffer[0]);

    // Draw the triangle
    glDrawElements(GL_TRIANGLES, lineIndexBuffer.size(), GL_UNSIGNED_INT, &lineIndexBuffer[0]);

    // Disable vertex array
    glDisableVertexAttribArray(positionHandle);
    glDisableVertexAttribArray(widthNormalHandle);
    glDisableVertexAttribArray(lengthNormalHandle);
    glDisableVertexAttribArray(pointAHandle);
    glDisableVertexAttribArray(pointBHandle);

    glDisable(GL_BLEND);
}

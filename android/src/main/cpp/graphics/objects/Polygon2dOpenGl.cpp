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

std::shared_ptr<MaskingObjectInterface> Polygon2dOpenGl::asMaskingObject() {
    return shared_from_this();
}

bool Polygon2dOpenGl::isReady() { return ready; }

void Polygon2dOpenGl::setPolygonPositions(const std::vector<::Vec2D> &positions, const std::vector<std::vector<::Vec2D>> &holes,
                                          bool isConvex) {
    polygonCoordinates = positions;
    holePolygonCoordinates = holes;
    polygonIsConvex = isConvex;
    ready = false;
}

void Polygon2dOpenGl::setup(const std::shared_ptr<::RenderingContextInterface> &context) {
    if (ready)
        return;

    std::shared_ptr<OpenGlContext> openGlContext = std::static_pointer_cast<OpenGlContext>(context);
    if (openGlContext->getProgram(shaderProgram->getProgramName()) == 0) {
        shaderProgram->setupProgram(openGlContext);
    }
    initializePolygon();
    ready = true;
}

void Polygon2dOpenGl::initializePolygon() {
    vertexBuffer.clear();
    indexBuffer.clear();

    for (auto &p : polygonCoordinates) {
        vertexBuffer.push_back(p.x);
        vertexBuffer.push_back(p.y);
        vertexBuffer.push_back(0.0);
    }

    for (size_t i = 0; i < polygonCoordinates.size() - 2; ++i) {
        indexBuffer.push_back(0);
        indexBuffer.push_back(i + 1);
        indexBuffer.push_back(i + 2);
    }

    auto startI = vertexBuffer.size() / 3;

    for (auto &h : holePolygonCoordinates) {
        for (auto &p : h) {
            vertexBuffer.push_back(p.x);
            vertexBuffer.push_back(p.y);
            vertexBuffer.push_back(0.0);
        }

        for (size_t i = 0; i < h.size() - 2; ++i) {
            indexBuffer.push_back(startI + 0);
            indexBuffer.push_back(startI + i + 1);
            indexBuffer.push_back(startI + i + 2);
        }

        startI = startI + h.size();
    }
}

void Polygon2dOpenGl::clear() {
    // TODO TOPO-1470: Clear GL-Data (careful with shared program/textures)
    ready = false;
}

void Polygon2dOpenGl::render(const std::shared_ptr<::RenderingContextInterface> &context, const RenderPassConfig &renderPass,
                             int64_t mvpMatrix, bool isMasked, double screenPixelAsRealMeterFactor) {
    if (!ready)
        return;

    std::shared_ptr<OpenGlContext> openGlContext = std::static_pointer_cast<OpenGlContext>(context);
    int program = openGlContext->getProgram(shaderProgram->getProgramName());

    glEnable(GL_STENCIL_TEST);

    // By drawing the triangle fan with indices around the polygon,
    // all the parts that need to be drawn are drawn an odd time.
    // Sections of intersecting holes, however, would be drawn as
    // well. In an all-convex case, the fragments of the original
    // polygon are only drawn once, in contrast to the ones in
    // holes. Non-convex polygons with intersecting holes will
    // need a more elaborate solution.
    // (Here we fill the stencil by always increasing the it by 1)
    glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
    glStencilFunc(GL_NEVER, 0, 63);
    glStencilOp(GL_INCR, GL_KEEP, GL_INCR);

    drawPolygon(openGlContext, program, mvpMatrix);

    if (polygonIsConvex) {
        // draw all the ones only
        glStencilFunc(GL_EQUAL, isMasked ? 129 : 1, 255);
    } else {
        // draw all the odd ones
        glStencilFunc(GL_EQUAL, isMasked ? 129 : 1, isMasked ? 129 : 1);
    }
    glStencilOp(GL_ZERO, GL_ZERO, GL_ZERO);
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

    drawPolygon(openGlContext, program, mvpMatrix);

    glDisable(GL_STENCIL_TEST);
}

void Polygon2dOpenGl::drawPolygon(std::shared_ptr<OpenGlContext> openGlContext, int program, int64_t mvpMatrix) {
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

    shaderProgram->preRender(openGlContext);

    // Prepare the triangle coordinate data
    glVertexAttribPointer(positionHandle, 3, GL_FLOAT, false, 12, &vertexBuffer[0]);

    // Draw the triangle
    glDrawElements(GL_TRIANGLES, (unsigned short)indexBuffer.size(), GL_UNSIGNED_SHORT, &indexBuffer[0]);

    // Disable vertex array
    glDisableVertexAttribArray(positionHandle);

    glDisable(GL_BLEND);
}

void Polygon2dOpenGl::renderAsMask(const std::shared_ptr<::RenderingContextInterface> &context, const ::RenderPassConfig &renderPass,
                                   int64_t mvpMatrix, double screenPixelAsRealMeterFactor) {
    if (!ready) return;

    std::shared_ptr<OpenGlContext> openGlContext = std::static_pointer_cast<OpenGlContext>(context);
    int program = openGlContext->getProgram(shaderProgram->getProgramName());

    glStencilFunc(GL_EQUAL, 128, 128);
    glStencilOp(GL_REPLACE, GL_ZERO, GL_ZERO);

    drawPolygon(openGlContext, program, mvpMatrix);
}

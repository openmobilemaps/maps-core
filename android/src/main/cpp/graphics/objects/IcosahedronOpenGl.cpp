/*
 * Copyright (c) 2021 Ubique Innovation AG <https://www.ubique.ch>
 *
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this
 *  file, You can obtain one at https://mozilla.org/MPL/2.0/.
 *
 *  SPDX-License-Identifier: MPL-2.0
 */

#include "IcosahedronOpenGl.h"
#include "OpenGlHelper.h"

IcosahedronOpenGl::IcosahedronOpenGl(const std::shared_ptr<::ShaderProgramInterface> &shader)
    : shaderProgram(shader) {}

std::shared_ptr<GraphicsObjectInterface> IcosahedronOpenGl::asGraphicsObject() { return shared_from_this(); }

bool IcosahedronOpenGl::isReady() { return ready; }

void IcosahedronOpenGl::setVertices(const ::SharedBytes & vertices_, const ::SharedBytes & indices_) {
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

void IcosahedronOpenGl::setup(const std::shared_ptr<::RenderingContextInterface> &context) {
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

void IcosahedronOpenGl::prepareGlData(int program) {
    glUseProgram(program);

    positionHandle = glGetAttribLocation(program, "vLatLon");
    valueHandle = glGetAttribLocation(program, "vValue");
    if (!glDataBuffersGenerated) {
        glGenBuffers(1, &vertexBuffer);
    }
    glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * vertices.size(), &vertices[0], GL_STATIC_DRAW);

    glBindBuffer(GL_ARRAY_BUFFER, 0);

    if (!glDataBuffersGenerated) {
        glGenBuffers(1, &indexBuffer);
    }
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuffer);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLuint) * indices.size(), &indices[0], GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    vpMatrixHandle = glGetUniformLocation(program, "uvpMatrix");
    mMatrixHandle = glGetUniformLocation(program, "umMatrix");
}

void IcosahedronOpenGl::clear() {
    std::lock_guard<std::recursive_mutex> lock(dataMutex);
    if (ready) {
        removeGlBuffers();
        ready = false;
    }
}

void IcosahedronOpenGl::removeGlBuffers() {
    if (glDataBuffersGenerated) {
        glDeleteBuffers(1, &vertexBuffer);
        glDeleteBuffers(1, &indexBuffer);
        glDataBuffersGenerated = false;
    }
}

void IcosahedronOpenGl::setIsInverseMasked(bool inversed) { isMaskInversed = inversed; }

void IcosahedronOpenGl::render(const std::shared_ptr<::RenderingContextInterface> &context, const RenderPassConfig &renderPass,
                             int64_t vpMatrix, int64_t mMatrix, bool isMasked, double screenPixelAsRealMeterFactor) {
    if (!ready)
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

    // Add program to OpenGL environment
    glUseProgram(program);

    shaderProgram->preRender(context);

    glEnableVertexAttribArray(positionHandle);
    glEnableVertexAttribArray(valueHandle);
    glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
    glVertexAttribPointer(positionHandle, 2, GL_FLOAT, false, sizeof(GLfloat) * 3, nullptr);
    glVertexAttribPointer(valueHandle, 1, GL_FLOAT, false, sizeof(GLfloat) * 3, (float *) (sizeof(GLfloat) * 2));

    glBindBuffer(GL_ARRAY_BUFFER, 0);

    // Apply the projection and view transformation
    glUniformMatrix4fv(vpMatrixHandle, 1, false, (GLfloat *)vpMatrix);
    glUniformMatrix4fv(mMatrixHandle, 1, false, (GLfloat *)mMatrix);

    // Draw the triangle
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuffer);
    glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, nullptr);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    // Disable vertex array
    glDisableVertexAttribArray(positionHandle);
    glDisableVertexAttribArray(valueHandle);

    glDisable(GL_BLEND);
}

void IcosahedronOpenGl::setDebugLabel(const std::string &label) {
    // not used
}

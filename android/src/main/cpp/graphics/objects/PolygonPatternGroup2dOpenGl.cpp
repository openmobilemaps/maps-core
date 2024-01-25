/*
 * Copyright (c) 2021 Ubique Innovation AG <https://www.ubique.ch>
 *
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this
 *  file, You can obtain one at https://mozilla.org/MPL/2.0/.
 *
 *  SPDX-License-Identifier: MPL-2.0
 */

#include "PolygonPatternGroup2dOpenGl.h"
#include "Logger.h"
#include "OpenGlHelper.h"
#include "TextureHolderInterface.h"
#include "BaseShaderProgramOpenGl.h"

PolygonPatternGroup2dOpenGl::PolygonPatternGroup2dOpenGl(const std::shared_ptr<::ShaderProgramInterface> &shader)
    : shaderProgram(shader) {}

bool PolygonPatternGroup2dOpenGl::isReady() { return ready && textureHolder && !buffersNotReady; }

std::shared_ptr<GraphicsObjectInterface> PolygonPatternGroup2dOpenGl::asGraphicsObject() { return shared_from_this(); }

void PolygonPatternGroup2dOpenGl::setIsInverseMasked(bool inversed) { isMaskInversed = inversed; }

void PolygonPatternGroup2dOpenGl::setVertices(const SharedBytes &vertices_, const SharedBytes &indices_) {
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

void PolygonPatternGroup2dOpenGl::setup(const std::shared_ptr<::RenderingContextInterface> &context) {
    if (ready || !dataReady)
        return;
    std::lock_guard<std::recursive_mutex> lock(dataMutex);

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

void PolygonPatternGroup2dOpenGl::prepareGlData(int program) {
    glUseProgram(program);

    positionHandle = glGetAttribLocation(program, "vPosition");
    styleIndexHandle = glGetAttribLocation(program, "vStyleIndex");
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
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLushort) * indices.size(), &indices[0], GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    mvpMatrixHandle = glGetUniformLocation(program, "uMVPMatrix");
}

void PolygonPatternGroup2dOpenGl::clear() {
    std::lock_guard<std::recursive_mutex> lock(dataMutex);
    if (ready) {
        removeGlBuffers();
    }
    if (textureHolder) {
        removeTexture();
    }
    ready = false;
}

void PolygonPatternGroup2dOpenGl::removeGlBuffers() {
    if (glDataBuffersGenerated) {
        glDeleteBuffers(1, &vertexBuffer);
        glDeleteBuffers(1, &indexBuffer);
        glDataBuffersGenerated = false;
    }
}

void PolygonPatternGroup2dOpenGl::loadTexture(const std::shared_ptr<::RenderingContextInterface> &context,
                               const std::shared_ptr<TextureHolderInterface> &textureHolder) {
    std::lock_guard<std::recursive_mutex> lock(dataMutex);
    if (textureHolder != nullptr) {
        texturePointer = textureHolder->attachToGraphics();

        factorHeight = textureHolder->getImageHeight() * 1.0f / textureHolder->getTextureHeight();
        factorWidth = textureHolder->getImageWidth() * 1.0f / textureHolder->getTextureWidth();

        this->textureHolder = textureHolder;
    }
}

void PolygonPatternGroup2dOpenGl::removeTexture() {
    std::lock_guard<std::recursive_mutex> lock(dataMutex);
    if (textureHolder) {
        textureHolder->clearFromGraphics();
        textureHolder = nullptr;
        texturePointer = -1;
    }
}

void PolygonPatternGroup2dOpenGl::renderAsMask(const std::shared_ptr<::RenderingContextInterface> &context, const RenderPassConfig &renderPass,
                                int64_t mvpMatrix, double screenPixelAsRealMeterFactor) {
    glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
    render(context, renderPass, mvpMatrix, false, screenPixelAsRealMeterFactor);
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
}

void PolygonPatternGroup2dOpenGl::render(const std::shared_ptr<::RenderingContextInterface> &context, const RenderPassConfig &renderPass,
                          int64_t mvpMatrix, bool isMasked, double screenPixelAsRealMeterFactor) {
    std::lock_guard<std::recursive_mutex> lock(dataMutex);
    if (!ready || buffersNotReady || !textureHolder) {
        return;
    }

    glUseProgram(program);

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

    prepareTextureDraw(program);

    auto textureFactorHandle = glGetUniformLocation(program, "uTextureFactor");
    glUniform2f(textureFactorHandle, factorWidth, factorHeight);

    auto scalingFactorHandle = glGetUniformLocation(program, "uScalingFactor");
    glUniform2f(scalingFactorHandle, scalingFactor.x, scalingFactor.y);

    auto screenPixelAsRealMeterFactorHandle = glGetUniformLocation(program, "uScreenPixelAsRealMeterFactor");
    if (screenPixelAsRealMeterFactorHandle >= 0) {
        glUniform1f(screenPixelAsRealMeterFactorHandle, screenPixelAsRealMeterFactor);
    }

    int textureCoordinatesHandle = glGetUniformLocation(program, "textureCoordinates");
    glUniform1fv(textureCoordinatesHandle, sizeTextureCoordinatesValuesArray, &textureCoordinates[0]);
    int opacitiesHandle = glGetUniformLocation(program, "opacities");
    glUniform1fv(opacitiesHandle, sizeOpacitiesValuesArray, &opacities[0]);

    shaderProgram->preRender(context);

    // enable vPosition attribs
    size_t floatSize = sizeof(GLfloat);
    size_t stride = 3 * floatSize;

    glEnableVertexAttribArray(positionHandle);
    glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
    glVertexAttribPointer(positionHandle, 2, GL_FLOAT, false, stride, nullptr);
    glEnableVertexAttribArray(styleIndexHandle);
    glVertexAttribPointer(styleIndexHandle, 1, GL_FLOAT, false, stride, (float *)(2 * floatSize));
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    // Apply the projection and view transformation
    glUniformMatrix4fv(mvpMatrixHandle, 1, false, (GLfloat *)mvpMatrix);

    // Draw the triangles
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuffer);
    glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_SHORT, nullptr);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    // Disable vertex array
    glDisableVertexAttribArray(positionHandle);
    glDisableVertexAttribArray(styleIndexHandle);

    glDisable(GL_BLEND);
}

void PolygonPatternGroup2dOpenGl::prepareTextureDraw(int program) {
    if (!textureHolder) {
        return;
    }

    // Set the active texture unit to texture unit 0.
    glActiveTexture(GL_TEXTURE0);

    // Bind the texture to this unit.
    glBindTexture(GL_TEXTURE_2D, (unsigned int)texturePointer);

    // Tell the texture uniform sampler to use this texture in the shader by binding to texture unit 0.
    int textureUniformHandle = glGetUniformLocation(program, "uTextureSampler");
    glUniform1i(textureUniformHandle, 0);
}

void PolygonPatternGroup2dOpenGl::setScalingFactor(float factor) {
    this->scalingFactor.x = factor;
    this->scalingFactor.y = factor;
}

void PolygonPatternGroup2dOpenGl::setScalingFactors(const ::Vec2F & factor) {
    this->scalingFactor = factor;
}

void PolygonPatternGroup2dOpenGl::setOpacities(const SharedBytes &values) {
    std::lock_guard<std::recursive_mutex> lock(dataMutex);
    if (values.elementCount == 0) {
        return;
    }
    this->opacities.resize(sizeOpacitiesValuesArray, 0.0);
    std::memcpy(this->opacities.data(), (void *)values.address, values.elementCount * values.bytesPerElement);
    buffersNotReady &= ~(1);
}

void PolygonPatternGroup2dOpenGl::setTextureCoordinates(const SharedBytes &textureCoordinates) {
    std::lock_guard<std::recursive_mutex> lock(dataMutex);
    if (textureCoordinates.elementCount == 0) {
        return;
    }
    this->textureCoordinates.resize(sizeTextureCoordinatesValuesArray, 0.0);
    std::memcpy(this->textureCoordinates.data(), (void *)textureCoordinates.address, textureCoordinates.elementCount * textureCoordinates.bytesPerElement);
    buffersNotReady &= ~(1 << 1);
}

void PolygonPatternGroup2dOpenGl::setDebugLabel(const std::string &label) {
    // not used
}

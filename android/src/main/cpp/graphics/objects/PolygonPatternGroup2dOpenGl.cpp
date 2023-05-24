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
    if (openGlContext->getProgram(shaderProgram->getProgramName()) == 0) {
        shaderProgram->setupProgram(openGlContext);
    }

    int program = openGlContext->getProgram(shaderProgram->getProgramName());
    prepareGlData(program);

    programHandle = program;
    ready = true;
}

void PolygonPatternGroup2dOpenGl::prepareGlData(const int &programHandle) {
    glUseProgram(programHandle);

    positionHandle = glGetAttribLocation(programHandle, "vPosition");
    styleIndexHandle = glGetAttribLocation(programHandle, "vStyleIndex");
    glGenBuffers(1, &vertexBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * vertices.size(), &vertices[0], GL_STATIC_DRAW);

    glBindBuffer(GL_ARRAY_BUFFER, 0);

    glGenBuffers(1, &indexBuffer);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuffer);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLushort) * indices.size(), &indices[0], GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    /*textureCoordinatesHandle = glGetProgramResourceIndex(programHandle, GL_SHADER_STORAGE_BLOCK, "textureCoordinatesBuffer");
    glGenBuffers(1, &textureCoordinatesBuffer);
    opacitiesHandle = glGetProgramResourceIndex(programHandle, GL_SHADER_STORAGE_BLOCK, "opacityBuffer");
    glGenBuffers(1, &opacitiesBuffer);*/

    mvpMatrixHandle = glGetUniformLocation(programHandle, "uMVPMatrix");
}

void PolygonPatternGroup2dOpenGl::clear() {
    std::lock_guard<std::recursive_mutex> lock(dataMutex);
    if (ready) {
        removeGlBuffers();
        buffersNotReady = 0b00000011;
    }
    if (textureHolder) {
        removeTexture();
    }
    ready = false;
}

void PolygonPatternGroup2dOpenGl::removeGlBuffers() {
    glDeleteBuffers(1, &vertexBuffer);
    glDeleteBuffers(1, &indexBuffer);

    /*glDeleteBuffers(1, &opacitiesBuffer);
    glDeleteBuffers(1, &textureCoordinatesBuffer);*/
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

    glUseProgram(programHandle);

    if (isMasked) {
        glStencilFunc(GL_EQUAL, isMaskInversed ? 0 : 128, 128);
        glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
    }

    std::shared_ptr<OpenGlContext> openGlContext = std::static_pointer_cast<OpenGlContext>(context);
    int program = openGlContext->getProgram(shaderProgram->getProgramName());
    glUseProgram(program);

    prepareTextureDraw(program);

    auto textureFactorHandle = glGetUniformLocation(programHandle, "uTextureFactor");
    glUniform2f(textureFactorHandle, factorWidth, factorHeight);

    auto scalingFactorHandle = glGetUniformLocation(programHandle, "uScalingFactor");
    glUniform1f(scalingFactorHandle, scalingFactor);

    /*glBindBuffer(GL_SHADER_STORAGE_BUFFER, textureCoordinatesBuffer);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, textureCoordinatesHandle, textureCoordinatesBuffer);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, opacitiesBuffer);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, opacitiesHandle, opacitiesBuffer);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);*/

    int textureCoordinatesHandle = glGetUniformLocation(program, "textureCoordinates");
    glUniform1fv(textureCoordinatesHandle, 5 * 32, &textureCoordinates[0]);
    int opacitiesHandle = glGetUniformLocation(program, "opacities");
    glUniform1fv(opacitiesHandle, 32, &opacities[0]);

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

    // Enable blending
    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

    // Draw the triangles
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuffer);
    glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_SHORT, nullptr);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    // Disable vertex array
    glDisableVertexAttribArray(positionHandle);
    glDisableVertexAttribArray(styleIndexHandle);

    glDisable(GL_BLEND);
}

void PolygonPatternGroup2dOpenGl::prepareTextureDraw(int programHandle) {
    if (!textureHolder) {
        return;
    }

    // Set the active texture unit to texture unit 0.
    glActiveTexture(GL_TEXTURE0);

    // Bind the texture to this unit.
    glBindTexture(GL_TEXTURE_2D, (unsigned int)texturePointer);

    // Tell the texture uniform sampler to use this texture in the shader by binding to texture unit 0.
    int textureUniformHandle = glGetUniformLocation(programHandle, "uTextureSampler");
    glUniform1i(textureUniformHandle, 0);
}

void PolygonPatternGroup2dOpenGl::setScalingFactor(float factor) {
    this->scalingFactor = factor;
}

void PolygonPatternGroup2dOpenGl::setOpacities(const SharedBytes &values) {
    std::lock_guard<std::recursive_mutex> lock(dataMutex);
/*    if (writeToShaderStorageBuffer(values, opacitiesBuffer)) {
        buffersNotReady &= ~(1);
    }*/
    if (values.elementCount == 0) {
        return;
    }
    this->opacities.resize(values.elementCount * values.bytesPerElement / 4);
    std::memcpy(this->opacities.data(), (void *)values.address, values.elementCount * values.bytesPerElement);
    buffersNotReady &= ~(1);
}

void PolygonPatternGroup2dOpenGl::setTextureCoordinates(const SharedBytes &textureCoordinates) {
    std::lock_guard<std::recursive_mutex> lock(dataMutex);
/*    if (writeToShaderStorageBuffer(textureCoordinates, textureCoordinatesBuffer)) {
        buffersNotReady &= ~(1 << 1);
    }*/
    if (textureCoordinates.elementCount == 0) {
        return;
    }
    this->textureCoordinates.resize(textureCoordinates.elementCount * textureCoordinates.bytesPerElement / 4);
    std::memcpy(this->textureCoordinates.data(), (void *)textureCoordinates.address, textureCoordinates.elementCount * textureCoordinates.bytesPerElement);
    buffersNotReady &= ~(1 << 1);
}

bool PolygonPatternGroup2dOpenGl::writeToShaderStorageBuffer(const ::SharedBytes &data, GLuint target) {
    if(!ready){
        // Writing to buffer before it was created
        return false;
    }
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, target);
    glBufferData(GL_SHADER_STORAGE_BUFFER, data.elementCount * data.bytesPerElement, (void *) data.address, GL_DYNAMIC_DRAW);
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
    return true;
}
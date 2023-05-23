/*
 * Copyright (c) 2021 Ubique Innovation AG <https://www.ubique.ch>
 *
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this
 *  file, You can obtain one at https://mozilla.org/MPL/2.0/.
 *
 *  SPDX-License-Identifier: MPL-2.0
 */

#include "Text2dInstancedOpenGl.h"
#include "Logger.h"
#include "OpenGlHelper.h"
#include "TextureHolderInterface.h"
#include "BaseShaderProgramOpenGl.h"

Text2dInstancedOpenGl::Text2dInstancedOpenGl(const std::shared_ptr<::ShaderProgramInterface> &shader)
        : shaderProgram(shader) {}

bool Text2dInstancedOpenGl::isReady() { return ready && (!usesTextureCoords || textureHolder); }

std::shared_ptr<GraphicsObjectInterface> Text2dInstancedOpenGl::asGraphicsObject() { return shared_from_this(); }

void Text2dInstancedOpenGl::clear() {
    std::lock_guard<std::recursive_mutex> lock(dataMutex);
    if (ready) {
        removeGlBuffers();
    }
    if (textureCoordsReady) {
        removeTextureCoordsGlBuffers();
    }
    if (textureHolder) {
        removeTexture();
    }
    ready = false;
}

void Text2dInstancedOpenGl::setIsInverseMasked(bool inversed) { isMaskInversed = inversed; }

void Text2dInstancedOpenGl::setFrame(const Quad2dD &frame) {
    std::lock_guard<std::recursive_mutex> lock(dataMutex);
    ready = false;
    this->frame = frame;
}

void Text2dInstancedOpenGl::setup(const std::shared_ptr<::RenderingContextInterface> &context) {
    if (ready)
        return;
    std::lock_guard<std::recursive_mutex> lock(dataMutex);

    float frameZ = 0;
    vertices = {
            (float)frame.topLeft.x,     (float)frame.topLeft.y,     frameZ,
            (float)frame.bottomLeft.x,  (float)frame.bottomLeft.y,  frameZ,
            (float)frame.bottomRight.x, (float)frame.bottomRight.y, frameZ,
            (float)frame.topRight.x,    (float)frame.topRight.y,    frameZ,
    };
    indices = {
            0, 1, 2, 0, 2, 3,
    };
    adjustTextureCoordinates();

    std::shared_ptr<OpenGlContext> openGlContext = std::static_pointer_cast<OpenGlContext>(context);
    if (openGlContext->getProgram(shaderProgram->getProgramName()) == 0) {
        shaderProgram->setupProgram(openGlContext);
    }

    int program = openGlContext->getProgram(shaderProgram->getProgramName());
    prepareGlData(openGlContext, program);
    prepareTextureCoordsGlData(openGlContext, program);

    programHandle = program;
    ready = true;
}

void Text2dInstancedOpenGl::prepareGlData(const std::shared_ptr<OpenGlContext> &openGlContext, const int &programHandle) {
    glUseProgram(programHandle);

    positionHandle = glGetAttribLocation(programHandle, "vPosition");
    glGenBuffers(1, &vertexBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * vertices.size(), &vertices[0], GL_STATIC_DRAW);

    glBindBuffer(GL_ARRAY_BUFFER, 0);

    glGenBuffers(1, &indexBuffer);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuffer);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLubyte) * indices.size(), &indices[0], GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    instPositionsHandle = glGetAttribLocation(programHandle, "aPosition");
    glGenBuffers(1, &positionsBuffer);
    instTextureCoordinatesHandle = glGetAttribLocation(programHandle, "aTexCoordinate");
    glGenBuffers(1, &textureCoordinatesListBuffer);
    instScalesHandle = glGetAttribLocation(programHandle, "aScale");
    glGenBuffers(1, &scalesBuffer);
    instRotationsHandle = glGetAttribLocation(programHandle, "aRotation");
    glGenBuffers(1, &rotationsBuffer);
    instStyleIndicesHandle = glGetAttribLocation(programHandle, "aStyleIndex");
    glGenBuffers(1, &styleIndicesBuffer);
    styleBufferHandle = glGetProgramResourceIndex(programHandle, GL_SHADER_STORAGE_BLOCK, "textInstancedStyleBuffer");
    glGenBuffers(1, &styleBuffer);


    glBindBuffer(GL_ARRAY_BUFFER, 0);

    mvpMatrixHandle = glGetUniformLocation(programHandle, "uMVPMatrix");
}

void Text2dInstancedOpenGl::prepareTextureCoordsGlData(const std::shared_ptr<OpenGlContext> &openGlContext, const int &programHandle) {
    glUseProgram(programHandle);

    if (textureCoordsReady) {
        removeTextureCoordsGlBuffers();
    }

    textureCoordinateHandle = glGetAttribLocation(programHandle, "texCoordinate");
    if (textureCoordinateHandle < 0) {
        usesTextureCoords = false;
        return;
    }
    glGenBuffers(1, &textureCoordsBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, textureCoordsBuffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * textureCoords.size(), &textureCoords[0], GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    usesTextureCoords = true;
    textureCoordsReady = true;
    OpenGlHelper::checkGlError("prepareTextureCoordsGlData");
}

void Text2dInstancedOpenGl::removeGlBuffers() {
    glDeleteBuffers(1, &vertexBuffer);
    glDeleteBuffers(1, &indexBuffer);

    glDeleteBuffers(1, &positionsBuffer);
    glDeleteBuffers(1, &styleIndicesBuffer);
    glDeleteBuffers(1, &scalesBuffer);
    glDeleteBuffers(1, &textureCoordinatesListBuffer);
    glDeleteBuffers(1, &rotationsBuffer);
    glDeleteBuffers(1, &styleBuffer);
}

void Text2dInstancedOpenGl::removeTextureCoordsGlBuffers() {
    if (textureCoordsReady) {
        glDeleteBuffers(1, &textureCoordsBuffer);
        textureCoordsReady = false;
    }
}

void Text2dInstancedOpenGl::loadTexture(const std::shared_ptr<::RenderingContextInterface> &context,
                                        const std::shared_ptr<TextureHolderInterface> &textureHolder) {
    std::lock_guard<std::recursive_mutex> lock(dataMutex);
    if (textureHolder != nullptr) {
        texturePointer = textureHolder->attachToGraphics();

        factorHeight = textureHolder->getImageHeight() * 1.0f / textureHolder->getTextureHeight();
        factorWidth = textureHolder->getImageWidth() * 1.0f / textureHolder->getTextureWidth();
        adjustTextureCoordinates();

        if (ready) {
            std::shared_ptr<OpenGlContext> openGlContext = std::static_pointer_cast<OpenGlContext>(context);
            int program = openGlContext->getProgram(shaderProgram->getProgramName());
            prepareTextureCoordsGlData(openGlContext, program);
        }
        this->textureHolder = textureHolder;
    }
}

void Text2dInstancedOpenGl::removeTexture() {
    std::lock_guard<std::recursive_mutex> lock(dataMutex);
    if (textureHolder) {
        textureHolder->clearFromGraphics();
        textureHolder = nullptr;
        texturePointer = -1;
        if (textureCoordsReady) {
            removeTextureCoordsGlBuffers();
        }
    }
}

void Text2dInstancedOpenGl::adjustTextureCoordinates() {
    const float tMinX = textureCoordinates.x;
    const float tMaxX = (textureCoordinates.x + textureCoordinates.width);
    const float tMinY = textureCoordinates.y;
    const float tMaxY = (textureCoordinates.y + textureCoordinates.height);

    textureCoords = {tMinX, tMinY, tMinX, tMaxY, tMaxX, tMaxY, tMaxX, tMinY};
}

void Text2dInstancedOpenGl::render(const std::shared_ptr<::RenderingContextInterface> &context, const RenderPassConfig &renderPass,
                                   int64_t mvpMatrix, bool isMasked, double screenPixelAsRealMeterFactor) {
    std::lock_guard<std::recursive_mutex> lock(dataMutex);
    if (!ready || (usesTextureCoords && !textureCoordsReady) || instanceCount == 0 || buffersNotReady) {
        return;
    }

    glUseProgram(programHandle);

    if (isMasked) {
        glStencilFunc(GL_EQUAL, isMaskInversed ? 0 : 128, 128);
        glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
    }
    std::shared_ptr<OpenGlContext> openGlContext = std::static_pointer_cast<OpenGlContext>(context);
    int mProgram = openGlContext->getProgram(shaderProgram->getProgramName());
    glUseProgram(mProgram);

    if (usesTextureCoords) {
        prepareTextureDraw(openGlContext, programHandle);

        glEnableVertexAttribArray(textureCoordinateHandle);
        glBindBuffer(GL_ARRAY_BUFFER, textureCoordsBuffer);
        glVertexAttribPointer(textureCoordinateHandle, 2, GL_FLOAT, false, 0, nullptr);

        auto textureFactorHandle = glGetUniformLocation(programHandle, "textureFactor");
        glUniform2f(textureFactorHandle, factorWidth, factorHeight);
    }
    glBindBuffer(GL_ARRAY_BUFFER, positionsBuffer);
    glVertexAttribPointer(instPositionsHandle, 2, GL_FLOAT, GL_FALSE, 0, nullptr);
    glEnableVertexAttribArray(instPositionsHandle);
    glVertexAttribDivisor(instPositionsHandle, 1);

    glBindBuffer(GL_ARRAY_BUFFER, textureCoordinatesListBuffer);
    glVertexAttribPointer(instTextureCoordinatesHandle, 4, GL_FLOAT, GL_FALSE, 0, nullptr);
    glEnableVertexAttribArray(instTextureCoordinatesHandle);
    glVertexAttribDivisor(instTextureCoordinatesHandle, 1);

    glBindBuffer(GL_ARRAY_BUFFER, scalesBuffer);
    glVertexAttribPointer(instScalesHandle, 2, GL_FLOAT, GL_FALSE, 0, nullptr);
    glEnableVertexAttribArray(instScalesHandle);
    glVertexAttribDivisor(instScalesHandle, 1);

    glBindBuffer(GL_ARRAY_BUFFER, rotationsBuffer);
    glVertexAttribPointer(instRotationsHandle, 1, GL_FLOAT, GL_FALSE, 0, nullptr);
    glEnableVertexAttribArray(instRotationsHandle);
    glVertexAttribDivisor(instRotationsHandle, 1);

    glBindBuffer(GL_ARRAY_BUFFER, styleIndicesBuffer);
    glVertexAttribIPointer(instStyleIndicesHandle, 1, GL_UNSIGNED_SHORT, 0, nullptr);
    glEnableVertexAttribArray(instStyleIndicesHandle);
    glVertexAttribDivisor(instStyleIndicesHandle, 1);

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, styleBuffer);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, styleBufferHandle, styleBuffer);

    shaderProgram->preRender(context);

    // enable vPosition attribs
    glEnableVertexAttribArray(positionHandle);
    glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
    glVertexAttribPointer(positionHandle, 3, GL_FLOAT, false, 0, nullptr);


    // Apply the projection and view transformation
    glUniformMatrix4fv(mvpMatrixHandle, 1, false, (GLfloat *)mvpMatrix);

    // Enable blending
    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

    // Draw the triangles
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuffer);
    glDrawElementsInstanced(GL_TRIANGLES,6, GL_UNSIGNED_BYTE, nullptr, instanceCount);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

    glVertexAttribDivisor(instPositionsHandle, 0);
    glVertexAttribDivisor(instTextureCoordinatesHandle, 0);
    glVertexAttribDivisor(instScalesHandle, 0);
    glVertexAttribDivisor(instRotationsHandle, 0);
    glVertexAttribDivisor(instStyleIndicesHandle, 0);

    // Disable vertex array
    glDisableVertexAttribArray(positionHandle);
    if (textureHolder) {
        glDisableVertexAttribArray(textureCoordinateHandle);
    }
    glDisableVertexAttribArray(instPositionsHandle);
    glDisableVertexAttribArray(instTextureCoordinatesHandle);
    glDisableVertexAttribArray(instScalesHandle);
    glDisableVertexAttribArray(instRotationsHandle);
    glDisableVertexAttribArray(instStyleIndicesHandle);

    glDisable(GL_BLEND);
}

void Text2dInstancedOpenGl::prepareTextureDraw(std::shared_ptr<OpenGlContext> &openGLContext, int programHandle) {
    if (!textureHolder) {
        return;
    }

    // Set the active texture unit to texture unit 0.
    glActiveTexture(GL_TEXTURE0);

    // Bind the texture to this unit.
    glBindTexture(GL_TEXTURE_2D, (unsigned int)texturePointer);

    // Tell the texture uniform sampler to use this texture in the shader by binding to texture unit 0.
    int textureUniformHandle = glGetUniformLocation(programHandle, "textureSampler");
    glUniform1i(textureUniformHandle, 0);
}

void Text2dInstancedOpenGl::setInstanceCount(int count) {
    this->instanceCount = count;
}

void Text2dInstancedOpenGl::setPositions(const SharedBytes &positions) {
    std::lock_guard<std::recursive_mutex> lock(dataMutex);
    if (writeToBuffer(positions, positionsBuffer)) {
        buffersNotReady &= ~(1);
    }
}

void Text2dInstancedOpenGl::setRotations(const SharedBytes &rotations) {
    std::lock_guard<std::recursive_mutex> lock(dataMutex);
    if (writeToBuffer(rotations, rotationsBuffer)) {
        buffersNotReady &= ~(1 << 1);
    }
}

void Text2dInstancedOpenGl::setScales(const SharedBytes &scales) {
    std::lock_guard<std::recursive_mutex> lock(dataMutex);
    if (writeToBuffer(scales, scalesBuffer)) {
        buffersNotReady &= ~(1 << 2);
    }
}

void Text2dInstancedOpenGl::setTextureCoordinates(const SharedBytes &textureCoordinates) {
    std::lock_guard<std::recursive_mutex> lock(dataMutex);
    if (writeToBuffer(textureCoordinates, textureCoordinatesListBuffer)) {
        buffersNotReady &= ~(1 << 3);
    }
}

void Text2dInstancedOpenGl::setStyleIndices(const ::SharedBytes &indices) {
    std::lock_guard<std::recursive_mutex> lock(dataMutex);
    if (writeToBuffer(indices, styleIndicesBuffer)) {
        buffersNotReady &= ~(1 << 4);
    }
}

void Text2dInstancedOpenGl::setStyles(const ::SharedBytes &values) {
    std::lock_guard<std::recursive_mutex> lock(dataMutex);
    if (!ready) {
        return;
    }
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, styleBuffer);
    glBufferData(GL_SHADER_STORAGE_BUFFER, values.elementCount * values.bytesPerElement, (void *) values.address, GL_DYNAMIC_DRAW);
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
    buffersNotReady &= ~(1 << 5);
}

bool Text2dInstancedOpenGl::writeToBuffer(const ::SharedBytes &data, GLuint target) {
    if(!ready){
        // Writing to buffer before it was created
        return false;
    }

    glBindBuffer(GL_ARRAY_BUFFER, target);
    glBufferData(GL_ARRAY_BUFFER, data.elementCount * data.bytesPerElement, (void *) data.address, GL_DYNAMIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    return true;
}
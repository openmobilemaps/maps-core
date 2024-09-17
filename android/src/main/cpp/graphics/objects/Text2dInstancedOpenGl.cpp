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

bool Text2dInstancedOpenGl::isReady() { return ready && (!usesTextureCoords || textureHolder) && !buffersNotReady; }

std::shared_ptr<GraphicsObjectInterface> Text2dInstancedOpenGl::asGraphicsObject() { return shared_from_this(); }

void Text2dInstancedOpenGl::clear() {
    std::lock_guard<std::recursive_mutex> lock(dataMutex);
    if (ready) {
        removeGlBuffers();
        buffersNotReady = 0b00111111;
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
    programName = shaderProgram->getProgramName();
    program = openGlContext->getProgram(programName);
    if (program == 0) {
        shaderProgram->setupProgram(openGlContext);
        program = openGlContext->getProgram(programName);
    }

    prepareGlData(program);
    prepareTextureCoordsGlData(program);

    ready = true;
}

void Text2dInstancedOpenGl::prepareGlData(int program) {
    glUseProgram(program);

    positionHandle = glGetAttribLocation(program, "vPosition");
    if (!glDataBuffersGenerated) {
        glGenBuffers(1, &vertexBuffer);
    }
    glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * vertices.size(), &vertices[0], GL_STATIC_DRAW);

    if (!glDataBuffersGenerated) {
        glGenBuffers(1, &dynamicInstanceDataBuffer);
        glGenBuffers(1, &styleBuffer);
    }
    glBindBuffer(GL_ARRAY_BUFFER, dynamicInstanceDataBuffer);
    glBufferData(GL_ARRAY_BUFFER, instanceCount * instValuesSizeBytes, nullptr, GL_DYNAMIC_DRAW);

    glBindBuffer(GL_ARRAY_BUFFER, 0);

    if (!glDataBuffersGenerated) {
        glGenBuffers(1, &indexBuffer);
    }
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuffer);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLubyte) * indices.size(), &indices[0], GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    instPositionsHandle = glGetAttribLocation(program, "aPosition");
    instTextureCoordinatesHandle = glGetAttribLocation(program, "aTexCoordinate");
    instScalesHandle = glGetAttribLocation(program, "aScale");
    instRotationsHandle = glGetAttribLocation(program, "aRotation");
    instStyleIndicesHandle = glGetAttribLocation(program, "aStyleIndex");
    styleBufferHandle = glGetProgramResourceIndex(program, GL_SHADER_STORAGE_BLOCK, "textInstancedStyleBuffer");

    glBindBuffer(GL_ARRAY_BUFFER, 0);

    mvpMatrixHandle = glGetUniformLocation(program, "uMVPMatrix");

    glDataBuffersGenerated = true;
}

void Text2dInstancedOpenGl::prepareTextureCoordsGlData(int program) {
    glUseProgram(program);

    if (textureCoordsReady) {
        removeTextureCoordsGlBuffers();
    }

    textureCoordinateHandle = glGetAttribLocation(program, "texCoordinate");
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
    if (glDataBuffersGenerated) {
        glDeleteBuffers(1, &vertexBuffer);
        glDeleteBuffers(1, &indexBuffer);
        glDeleteBuffers(1, &dynamicInstanceDataBuffer);
        glDeleteBuffers(1, &styleBuffer);
        glDataBuffersGenerated = false;
    }
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
    removeTexture();

    if (textureHolder != nullptr) {
        texturePointer = textureHolder->attachToGraphics();

        factorHeight = textureHolder->getImageHeight() * 1.0f / textureHolder->getTextureHeight();
        factorWidth = textureHolder->getImageWidth() * 1.0f / textureHolder->getTextureWidth();
        adjustTextureCoordinates();

        if (ready) {
            prepareTextureCoordsGlData(program);
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

    if (usesTextureCoords) {
        prepareTextureDraw(program);

        glEnableVertexAttribArray(textureCoordinateHandle);
        glBindBuffer(GL_ARRAY_BUFFER, textureCoordsBuffer);
        glVertexAttribPointer(textureCoordinateHandle, 2, GL_FLOAT, false, 0, nullptr);

        auto textureFactorHandle = glGetUniformLocation(program, "textureFactor");
        glUniform2f(textureFactorHandle, factorWidth, factorHeight);
    }

    glBindBuffer(GL_ARRAY_BUFFER, dynamicInstanceDataBuffer);
    glVertexAttribPointer(instPositionsHandle, 2, GL_FLOAT, GL_FALSE, 0, (float*)(instPositionsOffsetBytes * instanceCount));
    glEnableVertexAttribArray(instPositionsHandle);
    glVertexAttribDivisor(instPositionsHandle, 1);
    glVertexAttribPointer(instTextureCoordinatesHandle, 4, GL_FLOAT, GL_FALSE, 0, (float*)(instTextureCoordinatesOffsetBytes * instanceCount));
    glEnableVertexAttribArray(instTextureCoordinatesHandle);
    glVertexAttribDivisor(instTextureCoordinatesHandle, 1);
    glVertexAttribPointer(instScalesHandle, 2, GL_FLOAT, GL_FALSE, 0, (float*)(instScalesOffsetBytes * instanceCount));
    glEnableVertexAttribArray(instScalesHandle);
    glVertexAttribDivisor(instScalesHandle, 1);
    glVertexAttribPointer(instRotationsHandle, 1, GL_FLOAT, GL_FALSE, 0, (float*)(instRotationsOffsetBytes * instanceCount));
    glEnableVertexAttribArray(instRotationsHandle);
    glVertexAttribDivisor(instRotationsHandle, 1);
    glVertexAttribIPointer(instStyleIndicesHandle, 1, GL_UNSIGNED_SHORT, 0, (float*)(instStyleIndicesOffsetBytes * instanceCount));
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

    // Draw the triangles
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuffer);

    // Draw halos first
    auto isHaloHandle = glGetUniformLocation(program, "isHalo");
    glUniform1f(isHaloHandle, 1.0);
    glDrawElementsInstanced(GL_TRIANGLES,6, GL_UNSIGNED_BYTE, nullptr, instanceCount);

    // Draw non-halos second
    glUniform1f(isHaloHandle, 0.0);
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

void Text2dInstancedOpenGl::prepareTextureDraw(int program) {
    if (!textureHolder) {
        return;
    }

    // Set the active texture unit to texture unit 0.
    glActiveTexture(GL_TEXTURE0);

    // Bind the texture to this unit.
    glBindTexture(GL_TEXTURE_2D, (unsigned int)texturePointer);

    // Tell the texture uniform sampler to use this texture in the shader by binding to texture unit 0.
    int textureUniformHandle = glGetUniformLocation(program, "textureSampler");
    glUniform1i(textureUniformHandle, 0);
}

void Text2dInstancedOpenGl::setInstanceCount(int count) {
    this->instanceCount = count;
}

void Text2dInstancedOpenGl::setPositions(const SharedBytes &positions) {
    std::lock_guard<std::recursive_mutex> lock(dataMutex);
    if (writeToDynamicInstanceDataBuffer(positions, instPositionsOffsetBytes)) {
        buffersNotReady &= ~(1);
    }
}

void Text2dInstancedOpenGl::setRotations(const SharedBytes &rotations) {
    std::lock_guard<std::recursive_mutex> lock(dataMutex);
    if (writeToDynamicInstanceDataBuffer(rotations, instRotationsOffsetBytes)) {
        buffersNotReady &= ~(1 << 1);
    }
}

void Text2dInstancedOpenGl::setScales(const SharedBytes &scales) {
    std::lock_guard<std::recursive_mutex> lock(dataMutex);
    if (writeToDynamicInstanceDataBuffer(scales, instScalesOffsetBytes)) {
        buffersNotReady &= ~(1 << 2);
    }
}

void Text2dInstancedOpenGl::setTextureCoordinates(const SharedBytes &textureCoordinates) {
    std::lock_guard<std::recursive_mutex> lock(dataMutex);
    if (writeToDynamicInstanceDataBuffer(textureCoordinates, instTextureCoordinatesOffsetBytes)) {
        buffersNotReady &= ~(1 << 3);
    }
}

void Text2dInstancedOpenGl::setStyleIndices(const ::SharedBytes &indices) {
    std::lock_guard<std::recursive_mutex> lock(dataMutex);
    if (writeToDynamicInstanceDataBuffer(indices, instStyleIndicesOffsetBytes)) {
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

bool Text2dInstancedOpenGl::writeToDynamicInstanceDataBuffer(const ::SharedBytes &data, GLuint targetOffsetBytes) {
    if(!ready){
        // Writing to buffer before it was created
        return false;
    }

    glBindBuffer(GL_ARRAY_BUFFER, dynamicInstanceDataBuffer);
    glBufferSubData(GL_ARRAY_BUFFER, targetOffsetBytes * instanceCount, data.elementCount * data.bytesPerElement, (void *) data.address);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    return true;
}

void Text2dInstancedOpenGl::setDebugLabel(const std::string &label) {
    // not used
}

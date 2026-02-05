/*
 * Copyright (c) 2021 Ubique Innovation AG <https://www.ubique.ch>
 *
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this
 *  file, You can obtain one at https://mozilla.org/MPL/2.0/.
 *
 *  SPDX-License-Identifier: MPL-2.0
 */

#include "Quad2dStretchedInstancedOpenGl.h"
#include "Logger.h"
#include "OpenGlHelper.h"
#include "TextureHolderInterface.h"
#include "SharedBytes.h"
#include <cassert>

Quad2dStretchedInstancedOpenGl::Quad2dStretchedInstancedOpenGl(const std::shared_ptr<::BaseShaderProgramOpenGl> &shader)
    : shaderProgram(shader) {}

bool Quad2dStretchedInstancedOpenGl::isReady() { return ready && (!usesTextureCoords || textureHolder) && !buffersNotReady; }

std::shared_ptr<GraphicsObjectInterface> Quad2dStretchedInstancedOpenGl::asGraphicsObject() { return shared_from_this(); }

std::shared_ptr<MaskingObjectInterface> Quad2dStretchedInstancedOpenGl::asMaskingObject() { return shared_from_this(); }

void Quad2dStretchedInstancedOpenGl::clear() {
    std::lock_guard<std::recursive_mutex> lock(dataMutex);
    if (ready) {
        removeGlBuffers();
        buffersNotReady = buffersNotReadyResetValue;
    }
    if (textureCoordsReady) {
        removeTextureCoordsGlBuffers();
    }
    if (textureHolder) {
        removeTexture();
    }
    ready = false;
}

void Quad2dStretchedInstancedOpenGl::setIsInverseMasked(bool inversed) { isMaskInversed = inversed; }

void Quad2dStretchedInstancedOpenGl::setFrame(const Quad2dD &frame, const Vec3D &origin, bool is3d) {
    std::lock_guard<std::recursive_mutex> lock(dataMutex);
    ready = false;
    this->frame = frame;
    this->quadsOrigin = origin;
    this->is3d = is3d;
}

void Quad2dStretchedInstancedOpenGl::setup(const std::shared_ptr<::RenderingContextInterface> &context) {
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

void Quad2dStretchedInstancedOpenGl::prepareGlData(int program) {
    glUseProgram(program);

    if (!glDataBuffersGenerated) {
        glGenVertexArrays(1, &vao);
    }
    glBindVertexArray(vao);

    if (!glDataBuffersGenerated) {
        glGenBuffers(1, &vertexBuffer);
    }
    glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * vertices.size(), &vertices[0], GL_STATIC_DRAW);

    positionHandle = glGetAttribLocation(program, "vPosition");

    glEnableVertexAttribArray(positionHandle);
    glVertexAttribPointer(positionHandle, 3, GL_FLOAT, false, 0, nullptr);

    instPositionsHandle = glGetAttribLocation(program, "aPosition");
    instTextureCoordinatesHandle = glGetAttribLocation(program, "aTexCoordinate");
    instScalesHandle = glGetAttribLocation(program, "aScale");
    instRotationsHandle = glGetAttribLocation(program, "aRotation");
    instAlphasHandle = glGetAttribLocation(program, "aAlpha");
    instStretchScalesHandle = glGetAttribLocation(program, "aStretchScales");
    instStretchXsHandle = glGetAttribLocation(program, "aStretchX");
    instStretchYsHandle = glGetAttribLocation(program, "aStretchY");
    
    if (!glDataBuffersGenerated) {
        glGenBuffers(1, &dynamicInstanceDataBuffer);
    }
    glBindBuffer(GL_ARRAY_BUFFER, dynamicInstanceDataBuffer);
    setBufferInstanceCapacity(instanceCount);

    glEnableVertexAttribArray(instPositionsHandle);
    glVertexAttribDivisor(instPositionsHandle, 1);
    glEnableVertexAttribArray(instTextureCoordinatesHandle);
    glVertexAttribDivisor(instTextureCoordinatesHandle, 1);
    glEnableVertexAttribArray(instScalesHandle);
    glVertexAttribDivisor(instScalesHandle, 1);
    glEnableVertexAttribArray(instRotationsHandle);
    glVertexAttribDivisor(instRotationsHandle, 1);
    glEnableVertexAttribArray(instAlphasHandle);
    glVertexAttribDivisor(instAlphasHandle, 1);
    glEnableVertexAttribArray(instStretchScalesHandle);
    glVertexAttribDivisor(instStretchScalesHandle, 1);
    glEnableVertexAttribArray(instStretchXsHandle);
    glVertexAttribDivisor(instStretchXsHandle, 1);
    glEnableVertexAttribArray(instStretchYsHandle);
    glVertexAttribDivisor(instStretchYsHandle, 1);

    glBindBuffer(GL_ARRAY_BUFFER, 0);

    if (!glDataBuffersGenerated) {
        glGenBuffers(1, &indexBuffer);
    }
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuffer);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLubyte) * indices.size(), &indices[0], GL_STATIC_DRAW);

    glBindVertexArray(0);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    mMatrixHandle = glGetUniformLocation(program, "umMatrix");
    originOffsetHandle = glGetUniformLocation(program, "uOriginOffset");

    glDataBuffersGenerated = true;
}

void Quad2dStretchedInstancedOpenGl::prepareTextureCoordsGlData(int program) {
    glUseProgram(program);
    glBindVertexArray(vao);

    textureCoordinateHandle = glGetAttribLocation(program, "texCoordinate");
    if (textureCoordinateHandle < 0) {
        usesTextureCoords = false;
        return;
    }

    if (!texCoordBufferGenerated) {
        glGenBuffers(1, &textureCoordsBuffer);
    }

    glBindBuffer(GL_ARRAY_BUFFER, textureCoordsBuffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * textureCoords.size(), &textureCoords[0], GL_STATIC_DRAW);

    glEnableVertexAttribArray(textureCoordinateHandle);
    glVertexAttribPointer(textureCoordinateHandle, 2, GL_FLOAT, false, 0, nullptr);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    usesTextureCoords = true;
    textureCoordsReady = true;

    glBindVertexArray(0);
}

void Quad2dStretchedInstancedOpenGl::removeGlBuffers() {
    if (glDataBuffersGenerated) {
        glDeleteBuffers(1, &vertexBuffer);
        glDeleteBuffers(1, &indexBuffer);
        glDeleteBuffers(1, &dynamicInstanceDataBuffer);
        glDeleteVertexArrays(1, &vao);
        glDataBuffersGenerated = false;
    }
}

void Quad2dStretchedInstancedOpenGl::removeTextureCoordsGlBuffers() {
    if (textureCoordsReady) {
        if (texCoordBufferGenerated) {
            glDeleteBuffers(1, &textureCoordsBuffer);
            texCoordBufferGenerated = false;
        }
        textureCoordsReady = false;
    }
}

void Quad2dStretchedInstancedOpenGl::loadTexture(const std::shared_ptr<::RenderingContextInterface> &context,
                               const std::shared_ptr<TextureHolderInterface> &textureHolder) {
    std::lock_guard<std::recursive_mutex> lock(dataMutex);
    if (textureHolder != nullptr) {
        texturePointer = textureHolder->attachToGraphics();
        OpenGlHelper::generateMipmap(texturePointer);

        factorHeight = textureHolder->getImageHeight() * 1.0f / textureHolder->getTextureHeight();
        factorWidth = textureHolder->getImageWidth() * 1.0f / textureHolder->getTextureWidth();
        adjustTextureCoordinates();

        if (ready) {
            prepareTextureCoordsGlData(program);
        }
        this->textureHolder = textureHolder;
    }
}

void Quad2dStretchedInstancedOpenGl::removeTexture() {
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

void Quad2dStretchedInstancedOpenGl::adjustTextureCoordinates() {
    const float tMinX = textureCoordinates.x;
    const float tMaxX = (textureCoordinates.x + textureCoordinates.width);
    const float tMinY = textureCoordinates.y;
    const float tMaxY = (textureCoordinates.y + textureCoordinates.height);

    textureCoords = {tMinX, tMinY, tMinX, tMaxY, tMaxX, tMaxY, tMaxX, tMinY};
}

void Quad2dStretchedInstancedOpenGl::renderAsMask(const std::shared_ptr<::RenderingContextInterface> &context,
                                                  const RenderPassConfig &renderPass,
                                                  int64_t vpMatrix, int64_t mMatrix,
                                                  const ::Vec3D &origin, double screenPixelAsRealMeterFactor,
                                                  bool isScreenSpaceCoords) {
    glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
    render(context, renderPass, vpMatrix, mMatrix, origin, false, screenPixelAsRealMeterFactor, isScreenSpaceCoords);
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
}

void Quad2dStretchedInstancedOpenGl::render(const std::shared_ptr<::RenderingContextInterface> &context,
                                            const RenderPassConfig &renderPass,
                                            int64_t vpMatrix, int64_t mMatrix, const ::Vec3D &origin,
                                            bool isMasked, double screenPixelAsRealMeterFactor, bool isScreenSpaceCoords) {
    std::lock_guard<std::recursive_mutex> lock(dataMutex);
    if (!ready || (usesTextureCoords && !textureCoordsReady) || instanceCount == 0 || buffersNotReady || !shaderProgram->isRenderable()) {
        return;
    }

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

    glUseProgram(program);
    glBindVertexArray(vao);

    if (usesTextureCoords) {
        prepareTextureDraw(program);
        auto textureFactorHandle = glGetUniformLocation(program, "textureFactor");
        glUniform2f(textureFactorHandle, factorWidth, factorHeight);
    }

    shaderProgram->preRender(context, isScreenSpaceCoords);

    if(shaderProgram->usesModelMatrix()) {
        glUniformMatrix4fv(mMatrixHandle, 1, false, (GLfloat *) mMatrix);
    }

    glUniform4f(originOffsetHandle, quadsOrigin.x - origin.x, quadsOrigin.y - origin.y, quadsOrigin.z - origin.z, 0.0);

    // Draw the triangles
    glDrawElementsInstanced(GL_TRIANGLES,6, GL_UNSIGNED_BYTE, nullptr, instanceCount);

    glBindVertexArray(0);

    glDisable(GL_BLEND);

}

void Quad2dStretchedInstancedOpenGl::prepareTextureDraw(int program) {
    if (!textureHolder) {
        return;
    }

    // Set the active texture unit to texture unit 0.
    glActiveTexture(GL_TEXTURE0);

    // Bind the texture to this unit.
    glBindTexture(GL_TEXTURE_2D, (unsigned int)texturePointer);

    // Enable mipmap min filtering
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // Tell the texture uniform sampler to use this texture in the shader by binding to texture unit 0.
    int textureUniformHandle = glGetUniformLocation(program, "textureSampler");
    glUniform1i(textureUniformHandle, 0);
}

void Quad2dStretchedInstancedOpenGl::setInstanceCount(int count) {
    std::lock_guard<std::recursive_mutex> lock(dataMutex);
    if (glDataBuffersGenerated && count > bufferInstanceCapacity) {
        glBindVertexArray(vao);
        glBindBuffer(GL_ARRAY_BUFFER, dynamicInstanceDataBuffer);
        setBufferInstanceCapacity(count);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
    }
    this->instanceCount = count;
}

void Quad2dStretchedInstancedOpenGl::setPositions(const SharedBytes &positions) {
    std::lock_guard<std::recursive_mutex> lock(dataMutex);
    if (writeToDynamicInstanceDataBuffer(positions, (is3d ? instPositionsOffsetBytes3d : instPositionsOffsetBytes))) {
        buffersNotReady &= ~(1);
    }
}

void Quad2dStretchedInstancedOpenGl::setRotations(const SharedBytes &rotations) {
    std::lock_guard<std::recursive_mutex> lock(dataMutex);
    if (writeToDynamicInstanceDataBuffer(rotations, (is3d ? instRotationsOffsetBytes3d : instRotationsOffsetBytes))) {
        buffersNotReady &= ~(1 << 1);
    }
}

void Quad2dStretchedInstancedOpenGl::setScales(const SharedBytes &scales) {
    std::lock_guard<std::recursive_mutex> lock(dataMutex);
    if (writeToDynamicInstanceDataBuffer(scales, (is3d ? instScalesOffsetBytes3d : instScalesOffsetBytes))) {
        buffersNotReady &= ~(1 << 2);
    }
 }

void Quad2dStretchedInstancedOpenGl::setTextureCoordinates(const SharedBytes &textureCoordinates) {
    std::lock_guard<std::recursive_mutex> lock(dataMutex);
    if (writeToDynamicInstanceDataBuffer(textureCoordinates, (is3d ? instTextureCoordinatesOffsetBytes3d : instTextureCoordinatesOffsetBytes))) {
        buffersNotReady &= ~(1 << 3);
    }
}

void Quad2dStretchedInstancedOpenGl::setAlphas(const SharedBytes &values) {
    std::lock_guard<std::recursive_mutex> lock(dataMutex);
    if (writeToDynamicInstanceDataBuffer(values, (is3d ? instAlphasOffsetBytes3d : instAlphasOffsetBytes))) {
        buffersNotReady &= ~(1 << 4);
    }
}

void Quad2dStretchedInstancedOpenGl::setStretchInfos(const ::SharedBytes &values) {
    std::lock_guard<std::recursive_mutex> lock(dataMutex);
    if (writeToDynamicInstanceDataBuffer(values, (is3d ? instStretchInfoOffsetBytes3d : instStretchInfoOffsetBytes))) {
        buffersNotReady &= ~(1 << 5);
    }
}

bool Quad2dStretchedInstancedOpenGl::writeToDynamicInstanceDataBuffer(const ::SharedBytes &data, GLuint targetOffsetBytes) {
    if(!ready){
        // Writing to buffer before it was created
        return false;
    }
    glBindBuffer(GL_ARRAY_BUFFER, dynamicInstanceDataBuffer);
    glBufferSubData(GL_ARRAY_BUFFER, targetOffsetBytes * bufferInstanceCapacity, data.elementCount * data.bytesPerElement, (void *) data.address);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    return true;
}

void Quad2dStretchedInstancedOpenGl::setBufferInstanceCapacity(int count) {
#ifndef NDEBUG
    // must be called with buffers bound.
    GLint vaoBinding;
    glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &vaoBinding);
    assert(vaoBinding == vao);
    GLint arrayBufferBinding;
    glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &arrayBufferBinding);
    assert(arrayBufferBinding == dynamicInstanceDataBuffer);
#endif
    bufferInstanceCapacity = count;
    glBufferData(GL_ARRAY_BUFFER, bufferInstanceCapacity * (is3d ? instValuesSizeBytes3d : instValuesSizeBytes), nullptr, GL_DYNAMIC_DRAW);
    buffersNotReady = buffersNotReadyResetValue;

    // Update attribute pointers
    glVertexAttribPointer(instPositionsHandle, is3d ? 3 : 2, GL_FLOAT, GL_FALSE, 0, (float *)((is3d ? instPositionsOffsetBytes3d : instPositionsOffsetBytes) * bufferInstanceCapacity));
    glVertexAttribPointer(instTextureCoordinatesHandle, 4, GL_FLOAT, GL_FALSE, 0, (float *)((is3d ? instTextureCoordinatesOffsetBytes3d : instTextureCoordinatesOffsetBytes) * bufferInstanceCapacity));
    glVertexAttribPointer(instScalesHandle, 2, GL_FLOAT, GL_FALSE, 0, (float *)((is3d ? instScalesOffsetBytes3d : instScalesOffsetBytes) * bufferInstanceCapacity));
    glVertexAttribPointer(instRotationsHandle, 1, GL_FLOAT, GL_FALSE, 0, (float *)((is3d ? instRotationsOffsetBytes3d : instRotationsOffsetBytes) * bufferInstanceCapacity));
    glVertexAttribPointer(instAlphasHandle, 1, GL_FLOAT, GL_FALSE, 0, (float *)((is3d ? instAlphasOffsetBytes3d : instAlphasOffsetBytes) * bufferInstanceCapacity));
    glVertexAttribPointer(instStretchScalesHandle, 2, GL_FLOAT, GL_FALSE, 10 * sizeof(GLfloat), (float *)((is3d ? instStretchInfoOffsetBytes3d : instStretchInfoOffsetBytes) * bufferInstanceCapacity));
    glVertexAttribPointer(instStretchXsHandle, 4, GL_FLOAT, GL_FALSE, 10 * sizeof(GLfloat), (float *)((is3d ? instStretchInfoOffsetBytes3d : instStretchInfoOffsetBytes) * bufferInstanceCapacity + instStretchXsAddOffsetBytes));
    glVertexAttribPointer(instStretchYsHandle, 4, GL_FLOAT, GL_FALSE, 10 * sizeof(GLfloat), (float *)((is3d ? instStretchInfoOffsetBytes3d : instStretchInfoOffsetBytes) * bufferInstanceCapacity + instStretchYsAddOffsetBytes));
}

void Quad2dStretchedInstancedOpenGl::setDebugLabel(const std::string &label) {
    // not used
}

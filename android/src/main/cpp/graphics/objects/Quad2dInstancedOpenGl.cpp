/*
 * Copyright (c) 2021 Ubique Innovation AG <https://www.ubique.ch>
 *
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this
 *  file, You can obtain one at https://mozilla.org/MPL/2.0/.
 *
 *  SPDX-License-Identifier: MPL-2.0
 */

#include "Quad2dInstancedOpenGl.h"
#include "Logger.h"
#include "OpenGlHelper.h"
#include "TextureHolderInterface.h"
#include "BaseShaderProgramOpenGl.h"

Quad2dInstancedOpenGl::Quad2dInstancedOpenGl(const std::shared_ptr<::ShaderProgramInterface> &shader)
    : shaderProgram(shader) {}

bool Quad2dInstancedOpenGl::isReady() { return ready && (!usesTextureCoords || textureHolder) && !buffersNotReady; }

std::shared_ptr<GraphicsObjectInterface> Quad2dInstancedOpenGl::asGraphicsObject() { return shared_from_this(); }

std::shared_ptr<MaskingObjectInterface> Quad2dInstancedOpenGl::asMaskingObject() { return shared_from_this(); }

void Quad2dInstancedOpenGl::clear() {
    std::lock_guard<std::recursive_mutex> lock(dataMutex);
    if (ready) {
        removeGlBuffers();
        buffersNotReady = 0b00011111;
    }
    if (textureCoordsReady) {
        removeTextureCoordsGlBuffers();
    }
    if (textureHolder) {
        removeTexture();
    }
    ready = false;
}

void Quad2dInstancedOpenGl::setIsInverseMasked(bool inversed) { isMaskInversed = inversed; }

void Quad2dInstancedOpenGl::setFrame(const Quad2dD &frame) {
    std::lock_guard<std::recursive_mutex> lock(dataMutex);
    ready = false;
    this->frame = frame;
}

void Quad2dInstancedOpenGl::setup(const std::shared_ptr<::RenderingContextInterface> &context) {
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

void Quad2dInstancedOpenGl::prepareGlData(int program) {
    glUseProgram(program);

    positionHandle = glGetAttribLocation(program, "vPosition");
    if (!glDataBuffersGenerated) {
        glGenBuffers(1, &vertexBuffer);
    }
    glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * vertices.size(), &vertices[0], GL_STATIC_DRAW);

    if (!glDataBuffersGenerated) {
        glGenBuffers(1, &dynamicInstanceDataBuffer);
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
    instRotationsHandle = glGetAttribLocation(program, "aRotation");
    instTextureCoordinatesHandle = glGetAttribLocation(program, "aTexCoordinate");
    instScalesHandle = glGetAttribLocation(program, "aScale");
    instAlphasHandle = glGetAttribLocation(program, "aAlpha");

    vpMatrixHandle = glGetUniformLocation(program, "uvpMatrix");
    mMatrixHandle = glGetUniformLocation(program, "umMatrix");

    glDataBuffersGenerated = true;
}

void Quad2dInstancedOpenGl::prepareTextureCoordsGlData(int program) {
    glUseProgram(program);

    if (textureCoordsReady) {
        removeTextureCoordsGlBuffers();
    }

    textureCoordinateHandle = glGetAttribLocation(program, "vTexCoordinate");
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
}

void Quad2dInstancedOpenGl::removeGlBuffers() {
    if (glDataBuffersGenerated) {
        glDeleteBuffers(1, &vertexBuffer);
        glDeleteBuffers(1, &indexBuffer);
        glDeleteBuffers(1, &dynamicInstanceDataBuffer);
        glDataBuffersGenerated = false;
    }
}

void Quad2dInstancedOpenGl::removeTextureCoordsGlBuffers() {
    if (textureCoordsReady) {
        glDeleteBuffers(1, &textureCoordsBuffer);
        textureCoordsReady = false;
    }
}

void Quad2dInstancedOpenGl::loadTexture(const std::shared_ptr<::RenderingContextInterface> &context,
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

void Quad2dInstancedOpenGl::removeTexture() {
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

void Quad2dInstancedOpenGl::adjustTextureCoordinates() {
    const float tMinX = textureCoordinates.x;
    const float tMaxX = (textureCoordinates.x + textureCoordinates.width);
    const float tMinY = textureCoordinates.y;
    const float tMaxY = (textureCoordinates.y + textureCoordinates.height);

    textureCoords = {tMinX, tMinY, tMinX, tMaxY, tMaxX, tMaxY, tMaxX, tMinY};
}

void Quad2dInstancedOpenGl::renderAsMask(const std::shared_ptr<::RenderingContextInterface> &context, const RenderPassConfig &renderPass,
                                int64_t vpMatrix, int64_t mMatrix, double screenPixelAsRealMeterFactor) {
    glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
    render(context, renderPass, vpMatrix, mMatrix, false, screenPixelAsRealMeterFactor);
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
}

void Quad2dInstancedOpenGl::render(const std::shared_ptr<::RenderingContextInterface> &context, const RenderPassConfig &renderPass,
                          int64_t vpMatrix, int64_t mMatrix, bool isMasked, double screenPixelAsRealMeterFactor) {
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
    glVertexAttribPointer(instPositionsHandle, 2, GL_FLOAT, GL_FALSE, 0, (float *)(instPositionsOffsetBytes * instanceCount));
    glEnableVertexAttribArray(instPositionsHandle);
    glVertexAttribDivisor(instPositionsHandle, 1);
    glVertexAttribPointer(instRotationsHandle, 1, GL_FLOAT, GL_FALSE, 0, (float *)(instRotationsOffsetBytes * instanceCount));
    glEnableVertexAttribArray(instRotationsHandle);
    glVertexAttribDivisor(instRotationsHandle, 1);
    glVertexAttribPointer(instTextureCoordinatesHandle, 4, GL_FLOAT, GL_FALSE, 0, (float *)(instTextureCoordinatesOffsetBytes * instanceCount));
    glEnableVertexAttribArray(instTextureCoordinatesHandle);
    glVertexAttribDivisor(instTextureCoordinatesHandle, 1);
    glVertexAttribPointer(instScalesHandle, 2, GL_FLOAT, GL_FALSE, 0, (float *)(instScalesOffsetBytes * instanceCount));
    glEnableVertexAttribArray(instScalesHandle);
    glVertexAttribDivisor(instScalesHandle, 1);
    glVertexAttribPointer(instAlphasHandle, 1, GL_FLOAT, GL_FALSE, 0, (float *)(instAlphasOffsetBytes * instanceCount));
    glEnableVertexAttribArray(instAlphasHandle);
    glVertexAttribDivisor(instAlphasHandle, 1);

    shaderProgram->preRender(context);

    // enable vPosition attribs
    glEnableVertexAttribArray(positionHandle);
    glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
    glVertexAttribPointer(positionHandle, 3, GL_FLOAT, false, 0, nullptr);

    glBindBuffer(GL_ARRAY_BUFFER, 0);

    // Apply the projection and view transformation
    glUniformMatrix4fv(vpMatrixHandle, 1, false, (GLfloat *)vpMatrix);
    glUniformMatrix4fv(mMatrixHandle, 1, false, (GLfloat *)mMatrix);

    // Draw the triangles
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuffer);

    glDrawElementsInstanced(GL_TRIANGLES,6, GL_UNSIGNED_BYTE, nullptr, instanceCount);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    glVertexAttribDivisor(instPositionsHandle, 0);
    glVertexAttribDivisor(instRotationsHandle, 0);
    glVertexAttribDivisor(instTextureCoordinatesHandle, 0);
    glVertexAttribDivisor(instScalesHandle, 0);
    glVertexAttribDivisor(instAlphasHandle, 0);

    // Disable vertex array
    glDisableVertexAttribArray(positionHandle);
    if (textureHolder) {
        glDisableVertexAttribArray(textureCoordinateHandle);
    }
    glDisableVertexAttribArray(instPositionsHandle);
    glDisableVertexAttribArray(instRotationsHandle);
    glDisableVertexAttribArray(instTextureCoordinatesHandle);
    glDisableVertexAttribArray(instScalesHandle);
    glDisableVertexAttribArray(instAlphasHandle);

    glDisable(GL_BLEND);
}

void Quad2dInstancedOpenGl::prepareTextureDraw(int program) {
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

void Quad2dInstancedOpenGl::setInstanceCount(int count) {
    std::lock_guard<std::recursive_mutex> lock(dataMutex);
    ready = false;
    this->instanceCount = count;
}

void Quad2dInstancedOpenGl::setPositions(const SharedBytes &positions) {
    std::lock_guard<std::recursive_mutex> lock(dataMutex);
    if (writeToDynamicInstanceDataBuffer(positions, instPositionsOffsetBytes)) {
        buffersNotReady &= ~(1);
    }
}

void Quad2dInstancedOpenGl::setRotations(const SharedBytes &rotations) {
    std::lock_guard<std::recursive_mutex> lock(dataMutex);
    if (writeToDynamicInstanceDataBuffer(rotations, instRotationsOffsetBytes)) {
        buffersNotReady &= ~(1 << 1);
    }
}

void Quad2dInstancedOpenGl::setScales(const SharedBytes &scales) {
    std::lock_guard<std::recursive_mutex> lock(dataMutex);
    if (writeToDynamicInstanceDataBuffer(scales, instScalesOffsetBytes)) {
        buffersNotReady &= ~(1 << 2);
    }
 }

void Quad2dInstancedOpenGl::setTextureCoordinates(const SharedBytes &textureCoordinates) {
    std::lock_guard<std::recursive_mutex> lock(dataMutex);
    if (writeToDynamicInstanceDataBuffer(textureCoordinates, instTextureCoordinatesOffsetBytes)) {
        buffersNotReady &= ~(1 << 3);
    }
}

void Quad2dInstancedOpenGl::setAlphas(const SharedBytes &values) {
    std::lock_guard<std::recursive_mutex> lock(dataMutex);
    if (writeToDynamicInstanceDataBuffer(values, instAlphasOffsetBytes)) {
        buffersNotReady &= ~(1 << 4);
    }
}

bool Quad2dInstancedOpenGl::writeToDynamicInstanceDataBuffer(const ::SharedBytes &data, int targetOffsetBytes) {
    if(!ready){
        // Writing to buffer before it was created
        return false;
    }
    glBindBuffer(GL_ARRAY_BUFFER, dynamicInstanceDataBuffer);
    glBufferSubData(GL_ARRAY_BUFFER, targetOffsetBytes * instanceCount, data.elementCount * data.bytesPerElement, (void *) data.address);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    return true;
}

void Quad2dInstancedOpenGl::setDebugLabel(const std::string &label) {
    // not used
}

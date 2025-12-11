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
#include "TextInstancedShaderOpenGl.h"
#include <cassert>

Text2dInstancedOpenGl::Text2dInstancedOpenGl(const std::shared_ptr<::BaseShaderProgramOpenGl> &shader)
        : shaderProgram(shader) {}

bool Text2dInstancedOpenGl::isReady() { return ready && (!usesTextureCoords || textureHolder) && !buffersNotReady; }

std::shared_ptr<GraphicsObjectInterface> Text2dInstancedOpenGl::asGraphicsObject() { return shared_from_this(); }

void Text2dInstancedOpenGl::clear() {
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

void Text2dInstancedOpenGl::setIsInverseMasked(bool inversed) { isMaskInversed = inversed; }

void Text2dInstancedOpenGl::setFrame(const Quad2dD &frame, const Vec3D &origin, bool is3d) {
    std::lock_guard<std::recursive_mutex> lock(dataMutex);
    ready = false;
    this->frame = frame;
    this->quadsOrigin = origin;
    this->is3d = is3d;
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

    if (!glDataBuffersGenerated) {
        glGenBuffers(1, &dynamicInstanceDataBuffer);
    }
    glBindBuffer(GL_ARRAY_BUFFER, dynamicInstanceDataBuffer);
    glBufferData(GL_ARRAY_BUFFER, instanceCount * (is3d ? instValuesSizeBytes3d : instValuesSizeBytes), nullptr, GL_DYNAMIC_DRAW);

    instPositionsHandle = glGetAttribLocation(program, "aPosition");
    instTextureCoordinatesHandle = glGetAttribLocation(program, "aTexCoordinate");
    instScalesHandle = glGetAttribLocation(program, "aScale");
    instAlphasHandle = glGetAttribLocation(program, "aAlpha");
    instRotationsHandle = glGetAttribLocation(program, "aRotation");
    instStyleIndicesHandle = glGetAttribLocation(program, "aStyleIndex");
    instReferencePositionsHandle = glGetAttribLocation(program, "aReferencePosition");
    if (instReferencePositionsHandle < 0) {
        buffersNotReadyResetValue &= ~(1 << 6);
        buffersNotReady &= ~(1 << 6);
    }

    glVertexAttribPointer(instPositionsHandle, 2, GL_FLOAT, GL_FALSE, 0, (float*)((is3d ? instPositionsOffsetBytes3d : instPositionsOffsetBytes) * instanceCount));
    glEnableVertexAttribArray(instPositionsHandle);
    glVertexAttribDivisor(instPositionsHandle, 1);
    glVertexAttribPointer(instTextureCoordinatesHandle, 4, GL_FLOAT, GL_FALSE, 0, (float*)((is3d ? instTextureCoordinatesOffsetBytes3d : instTextureCoordinatesOffsetBytes) * instanceCount));
    glEnableVertexAttribArray(instTextureCoordinatesHandle);
    glVertexAttribDivisor(instTextureCoordinatesHandle, 1);
    glVertexAttribPointer(instScalesHandle, 2, GL_FLOAT, GL_FALSE, 0, (float*)((is3d ? instScalesOffsetBytes3d : instScalesOffsetBytes) * instanceCount));
    glEnableVertexAttribArray(instScalesHandle);
    glVertexAttribDivisor(instScalesHandle, 1);
    glVertexAttribPointer(instAlphasHandle, 1, GL_FLOAT, GL_FALSE, 0, (float*)((is3d ? instAlphasOffsetBytes3d : instAlphasOffsetBytes) * instanceCount));
    glEnableVertexAttribArray(instAlphasHandle);
    glVertexAttribDivisor(instAlphasHandle, 1);
    glVertexAttribPointer(instRotationsHandle, 1, GL_FLOAT, GL_FALSE, 0, (float*)((is3d ? instRotationsOffsetBytes3d : instRotationsOffsetBytes) * instanceCount));
    glEnableVertexAttribArray(instRotationsHandle);
    glVertexAttribDivisor(instRotationsHandle, 1);
    glVertexAttribIPointer(instStyleIndicesHandle, 1, GL_UNSIGNED_SHORT, 0, (float*)((is3d ? instStyleIndicesOffsetBytes3d : instStyleIndicesOffsetBytes) * instanceCount));
    glEnableVertexAttribArray(instStyleIndicesHandle);
    glVertexAttribDivisor(instStyleIndicesHandle, 1);
    if (instReferencePositionsHandle >= 0) {
        glVertexAttribPointer(instReferencePositionsHandle, is3d ? 3 : 2, GL_FLOAT, GL_FALSE, 0,
                              (float *) ((is3d ? instReferencePositionsOffsetBytes3d : instReferencePositionsOffsetBytes) * instanceCount));
        glEnableVertexAttribArray(instReferencePositionsHandle);
        glVertexAttribDivisor(instReferencePositionsHandle, 1);
    }

    glBindBuffer(GL_ARRAY_BUFFER, 0);

    if (!glDataBuffersGenerated) {
        glGenBuffers(1, &indexBuffer);
    }
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuffer);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLubyte) * indices.size(), &indices[0], GL_STATIC_DRAW);

    glBindVertexArray(0);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    if (!glDataBuffersGenerated) {
        glGenBuffers(1, &textStyleBuffer);
        glBindBuffer(GL_UNIFORM_BUFFER, textStyleBuffer);
        // maximum number of textStyles * size of padded style
        glBufferData(GL_UNIFORM_BUFFER, TextInstancedShaderOpenGl::BYTE_SIZE_TEXT_STYLES * TextInstancedShaderOpenGl::MAX_NUM_TEXT_STYLES,
                     nullptr,GL_DYNAMIC_DRAW);
        glBindBuffer(GL_UNIFORM_BUFFER, 0);
    }

    GLuint textStyleUniformBlockIdx = glGetUniformBlockIndex(program, "TextStyleCollection");
    if (textStyleUniformBlockIdx == GL_INVALID_INDEX) {
        LogError <<= "Uniform block TextStyleCollection not found";
    }
    glUniformBlockBinding(program, textStyleUniformBlockIdx, STYLE_UBO_BINDING);

    vpMatrixHandle = glGetUniformLocation(program, "uvpMatrix");
    mMatrixHandle = glGetUniformLocation(program, "umMatrix");
    originOffsetHandle = glGetUniformLocation(program, "uOriginOffset");
    if (is3d) {
        originHandle = glGetUniformLocation(program, "uOrigin");
    }
    isHaloHandle = glGetUniformLocation(program, "isHalo");
    textureFactorHandle = glGetUniformLocation(program, "textureFactor");
    distanceRangeHandle = glGetUniformLocation(program, "distanceRange");

    glDataBuffersGenerated = true;
}

void Text2dInstancedOpenGl::prepareTextureCoordsGlData(int program) {
    glUseProgram(program);
    glBindVertexArray(vao);

    textureCoordinateHandle = glGetAttribLocation(program, "texCoordinate");
    if (textureCoordinateHandle < 0) {
        usesTextureCoords = false;
        return;
    }

    if (!texCoordBufferGenerated) {
        glGenBuffers(1, &textureCoordsBuffer);
        texCoordBufferGenerated = true;
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

void Text2dInstancedOpenGl::removeGlBuffers() {
    if (glDataBuffersGenerated) {
        glDeleteBuffers(1, &vertexBuffer);
        glDeleteBuffers(1, &indexBuffer);
        glDeleteBuffers(1, &dynamicInstanceDataBuffer);
        if (textStyleBuffer > 0) {
            glDeleteBuffers(1, &textStyleBuffer);
        }
        glDeleteVertexArrays(1, &vao);
        glDataBuffersGenerated = false;
    }
}

void Text2dInstancedOpenGl::removeTextureCoordsGlBuffers() {
    if (textureCoordsReady) {
        if (texCoordBufferGenerated) {
            glDeleteBuffers(1, &textureCoordsBuffer);
            texCoordBufferGenerated = false;
        }
        textureCoordsReady = false;
    }
}

void Text2dInstancedOpenGl::loadFont(const std::shared_ptr<::RenderingContextInterface> &context,
                                     const ::FontData &fontData,
                                     const /*not-null*/ std::shared_ptr<TextureHolderInterface> &textureHolder) {
    std::lock_guard<std::recursive_mutex> lock(dataMutex);

    distanceRange = fontData.info.distanceRange;

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
        glUniform2f(textureFactorHandle, factorWidth, factorHeight);
    }

    glBindBufferBase(GL_UNIFORM_BUFFER, STYLE_UBO_BINDING, textStyleBuffer);

    shaderProgram->preRender(context, isScreenSpaceCoords);

    // Apply the projection and view transformation
    glUniformMatrix4fv(vpMatrixHandle, 1, false, (GLfloat *) vpMatrix);

    if(shaderProgram->usesModelMatrix()) {
        glUniformMatrix4fv(mMatrixHandle, 1, false, (GLfloat *) mMatrix);
    }

    glUniform4f(originOffsetHandle, quadsOrigin.x - origin.x, quadsOrigin.y - origin.y, quadsOrigin.z - origin.z, 0.0);
    if (is3d) {
        glUniform4f(originHandle, origin.x, origin.y, origin.z, 0.0);
    }
    glUniform1f(distanceRangeHandle, distanceRange);

    glUniform1f(distanceRangeHandle, distanceRange);

    // Draw halos first
    glUniform1f(isHaloHandle, 1.0);
    glDrawElementsInstanced(GL_TRIANGLES,6, GL_UNSIGNED_BYTE, nullptr, instanceCount);

    // Draw non-halos second
    glUniform1f(isHaloHandle, 0.0);
    glDrawElementsInstanced(GL_TRIANGLES,6, GL_UNSIGNED_BYTE, nullptr, instanceCount);

    glBindVertexArray(0);

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
    std::lock_guard<std::recursive_mutex> lock(dataMutex);
    this->instanceCount = count;
}

void Text2dInstancedOpenGl::setPositions(const SharedBytes &positions) {
    std::lock_guard<std::recursive_mutex> lock(dataMutex);
    if (writeToDynamicInstanceDataBuffer(positions, (is3d ? instPositionsOffsetBytes3d : instPositionsOffsetBytes))) {
        buffersNotReady &= ~(1);
    }
}

void Text2dInstancedOpenGl::setRotations(const SharedBytes &rotations) {
    std::lock_guard<std::recursive_mutex> lock(dataMutex);
    if (writeToDynamicInstanceDataBuffer(rotations, (is3d ? instRotationsOffsetBytes3d : instRotationsOffsetBytes))) {
        buffersNotReady &= ~(1 << 1);
    }
}

void Text2dInstancedOpenGl::setScales(const SharedBytes &scales) {
    std::lock_guard<std::recursive_mutex> lock(dataMutex);
    if (writeToDynamicInstanceDataBuffer(scales, (is3d ? instScalesOffsetBytes3d : instScalesOffsetBytes))) {
        buffersNotReady &= ~(1 << 2);
    }
}

void Text2dInstancedOpenGl::setAlphas(const SharedBytes &alphas) {
    std::lock_guard<std::recursive_mutex> lock(dataMutex);
    if (writeToDynamicInstanceDataBuffer(alphas, (is3d ? instAlphasOffsetBytes3d : instAlphasOffsetBytes))) {
        buffersNotReady &= ~(1 << 3);
    }
}

void Text2dInstancedOpenGl::setTextureCoordinates(const SharedBytes &textureCoordinates) {
    std::lock_guard<std::recursive_mutex> lock(dataMutex);
    if (writeToDynamicInstanceDataBuffer(textureCoordinates, (is3d ? instTextureCoordinatesOffsetBytes3d : instTextureCoordinatesOffsetBytes))) {
        buffersNotReady &= ~(1 << 4);
    }
}

void Text2dInstancedOpenGl::setStyleIndices(const ::SharedBytes &indices) {
    std::lock_guard<std::recursive_mutex> lock(dataMutex);
    if (writeToDynamicInstanceDataBuffer(indices, (is3d ? instStyleIndicesOffsetBytes3d : instStyleIndicesOffsetBytes))) {
        buffersNotReady &= ~(1 << 5);
    }
}

void Text2dInstancedOpenGl::setReferencePositions(const SharedBytes &positions) {
    std::lock_guard<std::recursive_mutex> lock(dataMutex);
    if (writeToDynamicInstanceDataBuffer(positions, (is3d ? instReferencePositionsOffsetBytes3d : instReferencePositionsOffsetBytes))) {
        buffersNotReady &= ~(1 << 6);
    }
}

void Text2dInstancedOpenGl::setStyles(const ::SharedBytes &values) {
    std::lock_guard<std::recursive_mutex> lock(dataMutex);
    if (!ready) {
        return;
    }
    assert(values.elementCount * values.bytesPerElement <= TextInstancedShaderOpenGl::MAX_NUM_TEXT_STYLES * TextInstancedShaderOpenGl::BYTE_SIZE_TEXT_STYLES);

    glBindBuffer(GL_UNIFORM_BUFFER, textStyleBuffer);
    glBufferSubData(GL_UNIFORM_BUFFER, 0, values.elementCount * values.bytesPerElement, (void *) values.address);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);

    buffersNotReady &= ~(1 << 7);
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

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
#include "BaseShaderProgramOpenGl.h"
#include "SharedBytes.h"

Quad2dStretchedInstancedOpenGl::Quad2dStretchedInstancedOpenGl(const std::shared_ptr<::ShaderProgramInterface> &shader)
    : shaderProgram(shader) {}

bool Quad2dStretchedInstancedOpenGl::isReady() { return ready && (!usesTextureCoords || textureHolder); }

std::shared_ptr<GraphicsObjectInterface> Quad2dStretchedInstancedOpenGl::asGraphicsObject() { return shared_from_this(); }

std::shared_ptr<MaskingObjectInterface> Quad2dStretchedInstancedOpenGl::asMaskingObject() { return shared_from_this(); }

void Quad2dStretchedInstancedOpenGl::clear() {
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

void Quad2dStretchedInstancedOpenGl::setIsInverseMasked(bool inversed) { isMaskInversed = inversed; }

void Quad2dStretchedInstancedOpenGl::setFrame(const Quad2dD &frame) {
    std::lock_guard<std::recursive_mutex> lock(dataMutex);
    ready = false;
    this->frame = frame;
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
    if (openGlContext->getProgram(shaderProgram->getProgramName()) == 0) {
        shaderProgram->setupProgram(openGlContext);
    }

    int program = openGlContext->getProgram(shaderProgram->getProgramName());
    prepareGlData(openGlContext, program);
    prepareTextureCoordsGlData(openGlContext, program);

    programHandle = program;
    ready = true;
}

void Quad2dStretchedInstancedOpenGl::prepareGlData(const std::shared_ptr<OpenGlContext> &openGlContext, const int &programHandle) {
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
    instAlphasHandle = glGetAttribLocation(programHandle, "aAlpha");
    glGenBuffers(1, &alphasBuffer);
    instStretchScalesHandle = glGetAttribLocation(programHandle, "aStretchScales");
    instStretchXsHandle = glGetAttribLocation(programHandle, "aStretchX");
    instStretchYsHandle = glGetAttribLocation(programHandle, "aStretchY");
    glGenBuffers(1, &stretchInfoBuffer);

    mvpMatrixHandle = glGetUniformLocation(programHandle, "uMVPMatrix");
}

void Quad2dStretchedInstancedOpenGl::prepareTextureCoordsGlData(const std::shared_ptr<OpenGlContext> &openGlContext, const int &programHandle) {
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
}

void Quad2dStretchedInstancedOpenGl::removeGlBuffers() {
    glDeleteBuffers(1, &vertexBuffer);
    glDeleteBuffers(1, &indexBuffer);

    glDeleteBuffers(1, &positionsBuffer);
    glDeleteBuffers(1, &alphasBuffer);
    glDeleteBuffers(1, &scalesBuffer);
    glDeleteBuffers(1, &textureCoordinatesListBuffer);
    glDeleteBuffers(1, &rotationsBuffer);
    glDeleteBuffers(1, &stretchInfoBuffer);
}

void Quad2dStretchedInstancedOpenGl::removeTextureCoordsGlBuffers() {
    if (textureCoordsReady) {
        glDeleteBuffers(1, &textureCoordsBuffer);
        textureCoordsReady = false;
    }
}

void Quad2dStretchedInstancedOpenGl::loadTexture(const std::shared_ptr<::RenderingContextInterface> &context,
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

void Quad2dStretchedInstancedOpenGl::renderAsMask(const std::shared_ptr<::RenderingContextInterface> &context, const RenderPassConfig &renderPass,
                                int64_t mvpMatrix, double screenPixelAsRealMeterFactor) {
    glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
    render(context, renderPass, mvpMatrix, false, screenPixelAsRealMeterFactor);
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
}

void Quad2dStretchedInstancedOpenGl::render(const std::shared_ptr<::RenderingContextInterface> &context, const RenderPassConfig &renderPass,
                          int64_t mvpMatrix, bool isMasked, double screenPixelAsRealMeterFactor) {
    if (!ready || (usesTextureCoords && !textureCoordsReady) || instanceCount == 0)
        return;
    OpenGlHelper::checkGlError("UBCM: Check pre render call");

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
    OpenGlHelper::checkGlError("UBCM: Check pre attributes");

    glBindBuffer(GL_ARRAY_BUFFER, positionsBuffer);
    glVertexAttribPointer(instPositionsHandle, 2, GL_FLOAT, GL_FALSE, 0, NULL);
    glEnableVertexAttribArray(instPositionsHandle);
    glVertexAttribDivisor(instPositionsHandle, 1);

    glBindBuffer(GL_ARRAY_BUFFER, textureCoordinatesListBuffer);
    glVertexAttribPointer(instTextureCoordinatesHandle, 4, GL_FLOAT, GL_FALSE, 0, NULL);
    glEnableVertexAttribArray(instTextureCoordinatesHandle);
    glVertexAttribDivisor(instTextureCoordinatesHandle, 1);

    glBindBuffer(GL_ARRAY_BUFFER, scalesBuffer);
    glVertexAttribPointer(instScalesHandle, 2, GL_FLOAT, GL_FALSE, 0, NULL);
    glEnableVertexAttribArray(instScalesHandle);
    glVertexAttribDivisor(instScalesHandle, 1);

    glBindBuffer(GL_ARRAY_BUFFER, rotationsBuffer);
    glVertexAttribPointer(instRotationsHandle, 1, GL_FLOAT, GL_FALSE, 0, NULL);
    glEnableVertexAttribArray(instRotationsHandle);
    glVertexAttribDivisor(instRotationsHandle, 1);

    glBindBuffer(GL_ARRAY_BUFFER, alphasBuffer);
    glVertexAttribPointer(instAlphasHandle, 1, GL_FLOAT, GL_FALSE, 0, NULL);
    glEnableVertexAttribArray(instAlphasHandle);
    glVertexAttribDivisor(instAlphasHandle, 1);

    glBindBuffer(GL_ARRAY_BUFFER, stretchInfoBuffer);
    glVertexAttribPointer(instStretchScalesHandle, 2, GL_FLOAT, GL_FALSE, 10 * sizeof(GLfloat), NULL);
    glEnableVertexAttribArray(instStretchScalesHandle);
    glVertexAttribDivisor(instStretchScalesHandle, 1);
    glVertexAttribPointer(instStretchXsHandle, 4, GL_FLOAT, GL_FALSE, 10 * sizeof(GLfloat), (float*) (2 * sizeof(GLfloat)));
    glEnableVertexAttribArray(instStretchXsHandle);
    glVertexAttribDivisor(instStretchXsHandle, 1);
    glVertexAttribPointer(instStretchYsHandle, 4, GL_FLOAT, GL_FALSE, 10 * sizeof(GLfloat), (float*) (6 * sizeof(GLfloat)));
    glEnableVertexAttribArray(instStretchYsHandle);
    glVertexAttribDivisor(instStretchYsHandle, 1);


    shaderProgram->preRender(context);
    OpenGlHelper::checkGlError("UBCM: Check post-pre-shader");

    // enable vPosition attribs
    glEnableVertexAttribArray(positionHandle);
    glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
    glVertexAttribPointer(positionHandle, 3, GL_FLOAT, false, 0, nullptr);

    glBindBuffer(GL_ARRAY_BUFFER, 0);

    // Apply the projection and view transformation
    glUniformMatrix4fv(mvpMatrixHandle, 1, false, (GLfloat *)mvpMatrix);

    // Enable blending
    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

    // Draw the triangles
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuffer);
    OpenGlHelper::checkGlError("UBCM: Check prerender");
    LogDebug <<= "UBCM: render stretched instances: " + std::to_string(instanceCount);
    glDrawElementsInstanced(GL_TRIANGLES,6, GL_UNSIGNED_BYTE, nullptr, instanceCount);
    std::stringstream ss;
    ss << "UBCM: Check postrender " << this;
    OpenGlHelper::checkGlError(ss.str());

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    glVertexAttribDivisor(instPositionsHandle, 0);
    glVertexAttribDivisor(instTextureCoordinatesHandle, 0);
    glVertexAttribDivisor(instScalesHandle, 0);
    glVertexAttribDivisor(instRotationsHandle, 0);
    glVertexAttribDivisor(instAlphasHandle, 0);
    glVertexAttribDivisor(instStretchScalesHandle, 0);
    glVertexAttribDivisor(instStretchXsHandle, 0);
    glVertexAttribDivisor(instStretchYsHandle, 0);

    OpenGlHelper::checkGlError("UBCM: Check post divisor");


    // Disable vertex array
    glDisableVertexAttribArray(positionHandle);
    if (textureHolder) {
        glDisableVertexAttribArray(textureCoordinateHandle);
    }
    glDisableVertexAttribArray(instPositionsHandle);
    glDisableVertexAttribArray(instTextureCoordinatesHandle);
    glDisableVertexAttribArray(instScalesHandle);
    glDisableVertexAttribArray(instRotationsHandle);
    glDisableVertexAttribArray(instAlphasHandle);
    glDisableVertexAttribArray(instStretchScalesHandle);
    glDisableVertexAttribArray(instStretchXsHandle);
    glDisableVertexAttribArray(instStretchYsHandle);

    OpenGlHelper::checkGlError("UBCM: Check post disable");

    glDisable(GL_BLEND);

    OpenGlHelper::checkGlError("UBCM: Check post-teardown");
}

void Quad2dStretchedInstancedOpenGl::prepareTextureDraw(std::shared_ptr<OpenGlContext> &openGLContext, int programHandle) {
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

void Quad2dStretchedInstancedOpenGl::setInstanceCount(int count) {
    this->instanceCount = count;
}

void Quad2dStretchedInstancedOpenGl::setPositions(const SharedBytes &positions) {
    OpenGlHelper::checkGlError("UBCM: Check before BindBuffer - positions");
    writeToBuffer(positions, positionsBuffer);
    LogDebug <<= "UBCM: write positions to buffer: " + std::to_string(positions.elementCount) + " elements (" + std::to_string(positions.bytesPerElement) + " * " + std::to_string(positions.elementCount) + ")";
    OpenGlHelper::checkGlError("UBCM: Check after BindBuffer - positions");
}

void Quad2dStretchedInstancedOpenGl::setRotations(const SharedBytes &rotations) {
    OpenGlHelper::checkGlError("UBCM: Check before BindBuffer - rotations");
    writeToBuffer(rotations, rotationsBuffer);
    LogDebug <<= "UBCM: write rotations to buffer: " + std::to_string(rotations.elementCount) + " elements (" + std::to_string(rotations.bytesPerElement) + " * " + std::to_string(rotations.elementCount) + ")";
    OpenGlHelper::checkGlError("UBCM: Check after BindBuffer - rotations");
}

void Quad2dStretchedInstancedOpenGl::setScales(const SharedBytes &scales) {
    OpenGlHelper::checkGlError("UBCM: Check before BindBuffer - scales");
    writeToBuffer(scales, scalesBuffer);
    LogDebug <<= "UBCM: write scales to buffer: " + std::to_string(scales.elementCount) + " elements (" + std::to_string(scales.bytesPerElement) + " * " + std::to_string(scales.elementCount) + ")";
    OpenGlHelper::checkGlError("UBCM: Check after BindBuffer - scales");
 }

void Quad2dStretchedInstancedOpenGl::setTextureCoordinates(const SharedBytes &textureCoordinates) {
    OpenGlHelper::checkGlError("UBCM: Check before BindBuffer - textureCoordinates");
    writeToBuffer(textureCoordinates, textureCoordinatesListBuffer);
    LogDebug <<= "UBCM: write textureCoordinates to buffer: " + std::to_string(textureCoordinates.elementCount) + " elements (" + std::to_string(textureCoordinates.bytesPerElement) + " * " + std::to_string(textureCoordinates.elementCount) + ")";
    OpenGlHelper::checkGlError("UBCM: Check after BindBuffer - textureCoordinates");
}

void Quad2dStretchedInstancedOpenGl::setAlphas(const SharedBytes &values) {
    OpenGlHelper::checkGlError("UBCM: Check before BindBuffer - alphas");
    writeToBuffer(values, alphasBuffer);
    LogDebug <<= "UBCM: write alphas to buffer: " + std::to_string(values.elementCount) + " elements (" + std::to_string(values.bytesPerElement) + " * " + std::to_string(values.elementCount) + ")";
    OpenGlHelper::checkGlError("UBCM: Check after BindBuffer - alphas");
}

void Quad2dStretchedInstancedOpenGl::setStretchInfos(const ::SharedBytes &values) {
    OpenGlHelper::checkGlError("UBCM: Check before BindBuffer - stretchInfos");
    writeToBuffer(values, stretchInfoBuffer);
    LogDebug <<= "UBCM: write stretchInfos to buffer: " + std::to_string(values.elementCount) + " elements (" + std::to_string(values.bytesPerElement) + " * " + std::to_string(values.elementCount) + ")";
    OpenGlHelper::checkGlError("UBCM: Check after BindBuffer - stretchInfos");
}

void Quad2dStretchedInstancedOpenGl::writeToBuffer(const ::SharedBytes &data, GLuint target) {
    if(!ready){
        LogError <<= "🥴 Writing to buffer before it was created.";
        return;
    }

    glBindBuffer(GL_ARRAY_BUFFER, target);
    glBufferData(GL_ARRAY_BUFFER, data.elementCount * data.bytesPerElement, (void *) data.address, GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}
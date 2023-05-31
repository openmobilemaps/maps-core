/*
 * Copyright (c) 2021 Ubique Innovation AG <https://www.ubique.ch>
 *
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this
 *  file, You can obtain one at https://mozilla.org/MPL/2.0/.
 *
 *  SPDX-License-Identifier: MPL-2.0
 */

#include "Text2dOpenGl.h"
#include "OpenGlHelper.h"
#include "TextureHolderInterface.h"

Text2dOpenGl::Text2dOpenGl(const std::shared_ptr<::ShaderProgramInterface> &shader)
    : shaderProgram(shader) {}

bool Text2dOpenGl::isReady() { return ready && textureHolder; }

std::shared_ptr<GraphicsObjectInterface> Text2dOpenGl::asGraphicsObject() { return shared_from_this(); }

void Text2dOpenGl::clear() {
    std::lock_guard<std::recursive_mutex> lock(dataMutex);
    if (ready) {
        removeGlBuffers();
    }
    if (textureHolder) {
        removeTexture();
    }
    ready = false;
}

void Text2dOpenGl::setIsInverseMasked(bool inversed) { isMaskInversed = inversed; }

void Text2dOpenGl::setTextsShared(const SharedBytes &vertices, const SharedBytes &indices) {
    std::lock_guard<std::recursive_mutex> lock(dataMutex);
    ready = false;
    dataReady = false;

    textIndices.resize(indices.elementCount);
    textVertexAttributes.resize(vertices.elementCount);
    if (indices.elementCount > 0) {
        std::memcpy(textIndices.data(), (void *) indices.address, indices.elementCount * indices.bytesPerElement);
    }
    if (vertices.elementCount > 0) {
        std::memcpy(textVertexAttributes.data(), (void *) vertices.address, vertices.elementCount * vertices.bytesPerElement);
    }

    dataReady = true;
}

void Text2dOpenGl::setTextsLegacy(const std::vector<TextDescription> &texts) {
    std::lock_guard<std::recursive_mutex> lock(dataMutex);
    ready = false;
    dataReady = false;

    std::vector<GlyphDescription> newGlyphs;
    for (const auto &text : texts) {
        for (const auto &glyph : text.glyphs) {
            newGlyphs.push_back(glyph);
        }
    }

    textVertexAttributes.clear();
    textIndices.clear();

    int iGlyph = 0;
    for (const auto &text : texts) {
        for (const auto &glyph : text.glyphs) {
            textVertexAttributes.push_back((float) glyph.frame.bottomLeft.x);
            textVertexAttributes.push_back((float) glyph.frame.bottomLeft.y);
            textVertexAttributes.push_back(glyph.textureCoordinates.bottomLeft.x);
            textVertexAttributes.push_back(glyph.textureCoordinates.bottomLeft.y);
            textVertexAttributes.push_back(0.0);
            textVertexAttributes.push_back(0.0);

            textVertexAttributes.push_back((float) glyph.frame.topLeft.x);
            textVertexAttributes.push_back((float) glyph.frame.topLeft.y);
            textVertexAttributes.push_back(glyph.textureCoordinates.topLeft.x);
            textVertexAttributes.push_back(glyph.textureCoordinates.topLeft.y);
            textVertexAttributes.push_back(0.0);
            textVertexAttributes.push_back(0.0);

            textVertexAttributes.push_back((float) glyph.frame.topRight.x);
            textVertexAttributes.push_back((float) glyph.frame.topRight.y);
            textVertexAttributes.push_back(glyph.textureCoordinates.topRight.x);
            textVertexAttributes.push_back(glyph.textureCoordinates.topRight.y);
            textVertexAttributes.push_back(0.0);
            textVertexAttributes.push_back(0.0);

            textVertexAttributes.push_back((float) glyph.frame.bottomRight.x);
            textVertexAttributes.push_back((float) glyph.frame.bottomRight.y);
            textVertexAttributes.push_back(glyph.textureCoordinates.bottomRight.x);
            textVertexAttributes.push_back(glyph.textureCoordinates.bottomRight.y);
            textVertexAttributes.push_back(0.0);
            textVertexAttributes.push_back(0.0);

            textIndices.push_back(iGlyph * 4);
            textIndices.push_back(iGlyph * 4 + 1);
            textIndices.push_back(iGlyph * 4 + 2);
            textIndices.push_back(iGlyph * 4);
            textIndices.push_back(iGlyph * 4 + 2);
            textIndices.push_back(iGlyph * 4 + 3);

            iGlyph++;
        }
    }

    dataReady = true;
}

void Text2dOpenGl::setup(const std::shared_ptr<::RenderingContextInterface> &context) {
    std::lock_guard<std::recursive_mutex> lock(dataMutex);
    if (ready || !dataReady) {
        return;
    }

    std::shared_ptr<OpenGlContext> openGlContext = std::static_pointer_cast<OpenGlContext>(context);
    int program = openGlContext->getProgram(shaderProgram->getProgramName());
    if (program == 0) {
        shaderProgram->setupProgram(openGlContext);
        program = openGlContext->getProgram(shaderProgram->getProgramName());
    }

    glUseProgram(program);
    prepareGlData(openGlContext, program);

    ready = true;
}

void Text2dOpenGl::prepareGlData(const std::shared_ptr<OpenGlContext> &openGlContext, const int &programHandle) {
    if (positionHandle < 0) {
        positionHandle = glGetAttribLocation(programHandle, "vPosition");
    }
    if (textureCoordinateHandle < 0) {
        textureCoordinateHandle = glGetAttribLocation(programHandle, "texCoordinate");
    }

    if (!hasVertexBuffer) {
        glGenBuffers(1, &vertexAttribBuffer);
        hasVertexBuffer = true;
    }
    glBindBuffer(GL_ARRAY_BUFFER, vertexAttribBuffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * textVertexAttributes.size(), &textVertexAttributes[0], GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    if (!hasIndexBuffer) {
        glGenBuffers(1, &indexBuffer);
        hasIndexBuffer = true;
    }
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuffer);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLushort) * textIndices.size(), &textIndices[0], GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    if (mvpMatrixHandle < 0) {
        mvpMatrixHandle = glGetUniformLocation(programHandle, "uMVPMatrix");
    }
    if (textureCoordScaleFactorHandle < 0) {
        textureCoordScaleFactorHandle = glGetUniformLocation(programHandle, "textureCoordScaleFactor");
    }
}

void Text2dOpenGl::removeGlBuffers() {
    std::lock_guard<std::recursive_mutex> lock(dataMutex);
    if (hasVertexBuffer) {
        glDeleteBuffers(1, &vertexAttribBuffer);
        hasVertexBuffer = false;
    }
    if (hasIndexBuffer) {
        glDeleteBuffers(1, &indexBuffer);
        hasIndexBuffer = false;
    }
}

void Text2dOpenGl::loadTexture(const std::shared_ptr<::RenderingContextInterface> &context,
                               const std::shared_ptr<TextureHolderInterface> &textureHolder) {
    std::lock_guard<std::recursive_mutex> lock(dataMutex);

    if (this->textureHolder) {
        removeTexture();
    }

    if (textureHolder != nullptr) {
        texturePointer = textureHolder->attachToGraphics();
        textureCoordScaleFactor = {textureHolder->getImageHeight() / (float) textureHolder->getTextureHeight(),
                                   textureHolder->getImageWidth() / (float) textureHolder->getTextureWidth()};
        this->textureHolder = textureHolder;
    }
}

void Text2dOpenGl::removeTexture() {
    std::lock_guard<std::recursive_mutex> lock(dataMutex);
    if (textureHolder) {
        textureHolder->clearFromGraphics();
        textureHolder = nullptr;
        texturePointer = -1;
        textureCoordScaleFactor = {1.0, 1.0};
    }
}

void Text2dOpenGl::renderAsMask(const std::shared_ptr<::RenderingContextInterface> &context, const RenderPassConfig &renderPass,
                                int64_t mvpMatrix, double screenPixelAsRealMeterFactor) {
    glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
    render(context, renderPass, mvpMatrix, false, screenPixelAsRealMeterFactor);
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
}

void Text2dOpenGl::render(const std::shared_ptr<::RenderingContextInterface> &context, const RenderPassConfig &renderPass,
                          int64_t mvpMatrix, bool isMasked, double screenPixelAsRealMeterFactor) {
    if (!ready || !textureHolder) {
        return;
    }

    if (isMasked) {
        glStencilFunc(GL_EQUAL, isMaskInversed ? 0 : 128, 128);
        glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
    }

    std::shared_ptr<OpenGlContext> openGlContext = std::static_pointer_cast<OpenGlContext>(context);
    int mProgram = openGlContext->getProgram(shaderProgram->getProgramName());
    glUseProgram(mProgram);

    prepareTextureDraw(openGlContext, mProgram);

    shaderProgram->preRender(context);

    // enable vPosition attribs
    glBindBuffer(GL_ARRAY_BUFFER, vertexAttribBuffer);
    glEnableVertexAttribArray(positionHandle);
    glVertexAttribPointer(positionHandle, 2, GL_FLOAT, false, 6 * sizeof(GLfloat), nullptr);
    // enable texture coord vertex attribs
    glEnableVertexAttribArray(textureCoordinateHandle);
    glVertexAttribPointer(textureCoordinateHandle, 2, GL_FLOAT, false, 6 * sizeof(GLfloat), (GLvoid*)(sizeof(GLfloat) * 2));
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    // Set texture coords scale factor
    glUniform2fv(textureCoordScaleFactorHandle, 1, &textureCoordScaleFactor[0]);

    // Apply the projection and view transformation
    glUniformMatrix4fv(mvpMatrixHandle, 1, false, (GLfloat *)mvpMatrix);

    // Draw the triangles
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuffer);
    glDrawElements(GL_TRIANGLES, textIndices.size(), GL_UNSIGNED_SHORT, nullptr);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    // Disable vertex array
    glDisableVertexAttribArray(positionHandle);
    glDisableVertexAttribArray(textureCoordinateHandle);

    if (textureHolder) {
        glDisableVertexAttribArray(textureCoordinateHandle);
    }

    glDisable(GL_BLEND);
}

void Text2dOpenGl::prepareTextureDraw(std::shared_ptr<OpenGlContext> &openGLContext, int mProgram) {
    if (!textureHolder) {
        return;
    }

    // Set the active texture unit to texture unit 0.
    glActiveTexture(GL_TEXTURE0);

    // Bind the texture to this unit.
    glBindTexture(GL_TEXTURE_2D, (unsigned int)texturePointer);

    // Tell the texture uniform sampler to use this texture in the shader by binding to texture unit 0.
    int mTextureUniformHandle = glGetUniformLocation(mProgram, "texture");
    glUniform1i(mTextureUniformHandle, 0);
}
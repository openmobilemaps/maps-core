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
#include <cstring>

Text2dOpenGl::Text2dOpenGl(const std::shared_ptr<::BaseShaderProgramOpenGl> &shader)
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
    programName = shaderProgram->getProgramName();
    program = openGlContext->getProgram(programName);
    if (program == 0) {
        shaderProgram->setupProgram(openGlContext);
        program = openGlContext->getProgram(programName);
        }

    prepareGlData(program);

    ready = true;
}

void Text2dOpenGl::prepareGlData(int program) {
    glUseProgram(program);

    if (!glDataBuffersGenerated) {
        glGenVertexArrays(1, &vao);
    }
    glBindVertexArray(vao);

    if (!glDataBuffersGenerated) {
        glGenBuffers(1, &vertexAttribBuffer);
    }
    glBindBuffer(GL_ARRAY_BUFFER, vertexAttribBuffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * textVertexAttributes.size(), &textVertexAttributes[0], GL_STATIC_DRAW);

    positionHandle = glGetAttribLocation(program, "vPosition");
    textureCoordinateHandle = glGetAttribLocation(program, "texCoordinate");

    glEnableVertexAttribArray(positionHandle);
    glVertexAttribPointer(positionHandle, 2, GL_FLOAT, false, 6 * sizeof(GLfloat), nullptr);
    glEnableVertexAttribArray(textureCoordinateHandle);
    glVertexAttribPointer(textureCoordinateHandle, 2, GL_FLOAT, false, 6 * sizeof(GLfloat), (GLvoid*)(sizeof(GLfloat) * 2));

    glBindBuffer(GL_ARRAY_BUFFER, 0);

    if (!glDataBuffersGenerated) {
        glGenBuffers(1, &indexBuffer);
    }
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuffer);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLushort) * textIndices.size(), &textIndices[0], GL_STATIC_DRAW);

    glBindVertexArray(0);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    if (mMatrixHandle < 0) {
        mMatrixHandle = glGetUniformLocation(program, "umMatrix");
    }
    if (textureCoordScaleFactorHandle < 0) {
        textureCoordScaleFactorHandle = glGetUniformLocation(program, "textureCoordScaleFactor");
    }

    glDataBuffersGenerated = true;
}

void Text2dOpenGl::removeGlBuffers() {
    std::lock_guard<std::recursive_mutex> lock(dataMutex);
    if (glDataBuffersGenerated) {
        glDeleteBuffers(1, &vertexAttribBuffer);
        glDeleteBuffers(1, &indexBuffer);
        glDeleteVertexArrays(1, &vao);
        glDataBuffersGenerated = false;
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
                                int64_t vpMatrix, int64_t mMatrix, const ::Vec3D &origin,
                                double screenPixelAsRealMeterFactor) {
    glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
    render(context, renderPass, vpMatrix, mMatrix, origin, false, screenPixelAsRealMeterFactor);
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
}

void Text2dOpenGl::render(const std::shared_ptr<::RenderingContextInterface> &context, const RenderPassConfig &renderPass,
                          int64_t vpMatrix, int64_t mMatrix, const ::Vec3D &origin, bool isMasked,
                          double screenPixelAsRealMeterFactor) {
    std::lock_guard<std::recursive_mutex> lock(dataMutex);
    if (!ready || !textureHolder || !shaderProgram->isRenderable()) {
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

    prepareTextureDraw(program);

    shaderProgram->preRender(context);

    // Set texture coords scale factor
    glUniform2fv(textureCoordScaleFactorHandle, 1, &textureCoordScaleFactor[0]);

    glUniformMatrix4fv(mMatrixHandle, 1, false, (GLfloat *)mMatrix);

    // Draw the triangles
    glDrawElements(GL_TRIANGLES, textIndices.size(), GL_UNSIGNED_SHORT, nullptr);

    glBindVertexArray(0);

    glDisable(GL_BLEND);
}

void Text2dOpenGl::prepareTextureDraw(int program) {
    if (!textureHolder) {
        return;
    }

    // Set the active texture unit to texture unit 0.
    glActiveTexture(GL_TEXTURE0);

    // Bind the texture to this unit.
    glBindTexture(GL_TEXTURE_2D, (unsigned int)texturePointer);

    // Tell the texture uniform sampler to use this texture in the shader by binding to texture unit 0.
    int mTextureUniformHandle = glGetUniformLocation(program, "texture");
    glUniform1i(mTextureUniformHandle, 0);
}

void Text2dOpenGl::setDebugLabel(const std::string &label) {
    // not used
}

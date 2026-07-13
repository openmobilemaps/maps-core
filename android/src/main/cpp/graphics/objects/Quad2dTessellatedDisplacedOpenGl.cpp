/*
 * Copyright (c) 2021 Ubique Innovation AG <https://www.ubique.ch>
 *
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this
 *  file, You can obtain one at https://mozilla.org/MPL/2.0/.
 *
 *  SPDX-License-Identifier: MPL-2.0
 */

#include "Quad2dTessellatedDisplacedOpenGl.h"
#include "TextureHolderInterface.h"

Quad2dTessellatedDisplacedOpenGl::Quad2dTessellatedDisplacedOpenGl(
    const std::shared_ptr<::BaseShaderProgramOpenGl> &shader)
    : Quad2dTessellatedOpenGl(shader) {}

bool Quad2dTessellatedDisplacedOpenGl::isReady() {
    return ready && (!usesTextureCoords || textureAttachment.isAttached());
}

void Quad2dTessellatedDisplacedOpenGl::render(const std::shared_ptr<::RenderingContextInterface> &context,
                                              const ::RenderPassConfig &renderPass, int64_t vpMatrix, int64_t mMatrix,
                                              const ::Vec3D &origin, bool isMasked,
                                              double screenPixelAsRealMeterFactor, bool isScreenSpaceCoords) {
    enableDepthTest();

    Quad2dTessellatedOpenGl::render(context, renderPass, vpMatrix, mMatrix, origin, isMasked,
                                    screenPixelAsRealMeterFactor, isScreenSpaceCoords);

    disableDepthTest();
}

void Quad2dTessellatedDisplacedOpenGl::loadDualTexture(
    const std::shared_ptr<::RenderingContextInterface> &context,
    const std::shared_ptr<TextureHolderInterface> &textureHolder,
    const std::shared_ptr<TextureHolderInterface> &elevationHolder) {
    std::lock_guard<std::recursive_mutex> lock(dataMutex);


    const bool newlyAttached = textureAttachment.attach(textureHolder);
    if (newlyAttached) {
        computeGeometry(true);

        if (ready) {
            prepareTextureCoordsGlData(program);
        }
    }

    lookupTextureAttachment.clear();
    elevationTexture.attach(elevationHolder);
}

void Quad2dTessellatedDisplacedOpenGl::loadTextures(
    const std::shared_ptr<::RenderingContextInterface> &context,
    const std::shared_ptr<TextureHolderInterface> &textureHolder,
    const std::shared_ptr<TextureHolderInterface> &lookupHolder,
    const std::shared_ptr<TextureHolderInterface> &elevationHolder) {
    std::lock_guard<std::recursive_mutex> lock(dataMutex);

    const bool newlyAttached = textureAttachment.attach(textureHolder);
    if (newlyAttached) {
        computeGeometry(true);

        if (ready) {
            prepareTextureCoordsGlData(program);
        }
    }

    lookupTextureAttachment.attach(lookupHolder);
    elevationTexture.attach(elevationHolder);
}

int Quad2dTessellatedDisplacedOpenGl::getLookupTextureUnit() const {
    // Elevation occupies texture unit 1 (elevationTextureSampler); keep the lookup sprite on a free unit.
    return 2;
}

bool Quad2dTessellatedDisplacedOpenGl::disablesDepthTestBeforeRender() const {
    return false;
}

void Quad2dTessellatedDisplacedOpenGl::pause() {
    if (!clearOnPause) {
        return;
    }

    std::lock_guard<std::recursive_mutex> lock(dataMutex);
    Quad2dTessellatedOpenGl::pause();

    elevationTexture.detach();
}

void Quad2dTessellatedDisplacedOpenGl::resume(const std::shared_ptr<::RenderingContextInterface> &context) {
    if (!clearOnPause) {
        return;
    }

    std::lock_guard<std::recursive_mutex> lock(dataMutex);
    Quad2dTessellatedOpenGl::resume(context);
    elevationTexture.attach();
}

void Quad2dTessellatedDisplacedOpenGl::removeTexture() {
    std::lock_guard<std::recursive_mutex> lock(dataMutex);

    elevationTexture.clear();
    Quad2dTessellatedOpenGl::removeTexture();
}

void Quad2dTessellatedDisplacedOpenGl::prepareGlData(int program) {
    Quad2dTessellatedOpenGl::prepareGlData(program);
    elevationTextureUniformHandle = glGetUniformLocation(program, "elevationTextureSampler");
    hasElevationTextureHandle = glGetUniformLocation(program, "uHasElevationTexture");
}

void Quad2dTessellatedDisplacedOpenGl::prepareTextureDraw(int program) {
    Quad2dTessellatedOpenGl::prepareTextureDraw(program);

    if (hasElevationTextureHandle >= 0) {
        glUniform1i(hasElevationTextureHandle, elevationTexture.isAttached());
    }

    if (!elevationTexture.isAttached() || elevationTextureUniformHandle < 0) {
        return;
    }

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, elevationTexture.texture());
    if (textureFilterType.has_value()) {
        GLint filterParam = *textureFilterType == TextureFilterType::LINEAR ? GL_LINEAR : GL_NEAREST;
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filterParam);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filterParam);
    }
    glUniform1i(elevationTextureUniformHandle, 1);
    glActiveTexture(GL_TEXTURE0);
}

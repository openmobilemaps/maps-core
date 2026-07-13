/*
 * Copyright (c) 2021 Ubique Innovation AG <https://www.ubique.ch>
 *
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this
 *  file, You can obtain one at https://mozilla.org/MPL/2.0/.
 *
 *  SPDX-License-Identifier: MPL-2.0
 */

#pragma once

#include "Quad2dTessellatedOpenGl.h"

class Quad2dTessellatedDisplacedOpenGl : public Quad2dTessellatedOpenGl {
public:
    explicit Quad2dTessellatedDisplacedOpenGl(const std::shared_ptr<::BaseShaderProgramOpenGl> &shader);

    bool isReady() override;

    void render(const std::shared_ptr<::RenderingContextInterface> &context, const ::RenderPassConfig &renderPass,
                int64_t vpMatrix, int64_t mMatrix, const ::Vec3D &origin, bool isMasked,
                double screenPixelAsRealMeterFactor, bool isScreenSpaceCoords) override;

    void loadDualTexture(const std::shared_ptr<::RenderingContextInterface> &context,
                         const std::shared_ptr<TextureHolderInterface> &textureHolder,
                         const std::shared_ptr<TextureHolderInterface> &elevationHolder) override;

    void loadTextures(const std::shared_ptr<::RenderingContextInterface> &context,
                      const std::shared_ptr<TextureHolderInterface> &textureHolder,
                      const std::shared_ptr<TextureHolderInterface> &lookupHolder,
                      const std::shared_ptr<TextureHolderInterface> &elevationHolder) override;

    void pause() override;

    void resume(const std::shared_ptr<::RenderingContextInterface> &context) override;

    void removeTexture() override;

protected:
    bool disablesDepthTestBeforeRender() const override;

    void prepareGlData(int program) override;

    void prepareTextureDraw(int program) override;

    int getLookupTextureUnit() const override;

    TextureAttachment elevationTexture;

    int elevationTextureUniformHandle = -1;
    int hasElevationTextureHandle = -1;
};

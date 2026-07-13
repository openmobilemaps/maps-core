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

#include "BaseShaderProgramOpenGl.h"
#include "Quad2dOpenGl.h"
#include "TexturedPolygon2dTessellatedOpenGl.h"
#include "TexturedPolygonInterface.h"
#include <memory>

class TexturedPolygonOpenGl : public TexturedPolygonInterface {
  public:
    explicit TexturedPolygonOpenGl(const std::shared_ptr<::BaseShaderProgramOpenGl> &shader);

    void setVertices(const ::SharedBytes &vertices, const ::SharedBytes &indices, const ::Vec3D &origin, bool is3d) override;

    void setSubdivisionFactor(int32_t factor) override;

    void setMinMagFilter(TextureFilterType filterType) override;

    void loadTexture(const std::shared_ptr<::RenderingContextInterface> &context,
                     const std::shared_ptr<TextureHolderInterface> &textureHolder) override;

    void loadDualTexture(const std::shared_ptr<::RenderingContextInterface> &context,
                         const std::shared_ptr<TextureHolderInterface> &textureHolder,
                         const std::shared_ptr<TextureHolderInterface> &elevationHolder) override;

    void removeTexture() override;

    std::shared_ptr<GraphicsObjectInterface> asGraphicsObject() override;

  private:
    std::shared_ptr<Quad2dOpenGl> quadGraphicsObject;
    std::shared_ptr<TexturedPolygon2dTessellatedOpenGl> tessellatedGraphicsObject;
};

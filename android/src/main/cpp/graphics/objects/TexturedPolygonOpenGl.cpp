/*
 * Copyright (c) 2021 Ubique Innovation AG <https://www.ubique.ch>
 *
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this
 *  file, You can obtain one at https://mozilla.org/MPL/2.0/.
 *
 *  SPDX-License-Identifier: MPL-2.0
 */

#include "TexturedPolygonOpenGl.h"
#include "TessellatedDisplacedRasterShaderOpenGl.h"
#include "TessellatedRasterShaderOpenGl.h"

TexturedPolygonOpenGl::TexturedPolygonOpenGl(const std::shared_ptr<::BaseShaderProgramOpenGl> &shader) {
    auto tessellatedShader = std::dynamic_pointer_cast<TessellatedRasterShaderOpenGl>(shader);
    auto displacedShader = std::dynamic_pointer_cast<TessellatedDisplacedRasterShaderOpenGl>(shader);
    if (tessellatedShader && !displacedShader) {
        tessellatedGraphicsObject = std::make_shared<TexturedPolygon2dTessellatedOpenGl>(tessellatedShader);
    } else {
        quadGraphicsObject = std::make_shared<Quad2dOpenGl>(shader);
    }
}

void TexturedPolygonOpenGl::setVertices(const ::SharedBytes &vertices,
                                          const ::SharedBytes &indices,
                                          const ::Vec3D &origin,
                                          bool is3d) {
    if (tessellatedGraphicsObject) {
        tessellatedGraphicsObject->setVertices(vertices, indices, origin, is3d);
    } else if (quadGraphicsObject) {
        quadGraphicsObject->setCustomGeometry(vertices, indices, origin, is3d);
    }
}

void TexturedPolygonOpenGl::setSubdivisionFactor(int32_t factor) {
    if (tessellatedGraphicsObject) {
        tessellatedGraphicsObject->setSubdivisionFactor(factor);
    } else if (quadGraphicsObject) {
        quadGraphicsObject->setSubdivisionFactor(factor);
    }
}

void TexturedPolygonOpenGl::setMinMagFilter(TextureFilterType filterType) {
    if (tessellatedGraphicsObject) {
        tessellatedGraphicsObject->setMinMagFilter(filterType);
    } else if (quadGraphicsObject) {
        quadGraphicsObject->setMinMagFilter(filterType);
    }
}

void TexturedPolygonOpenGl::loadTexture(const std::shared_ptr<::RenderingContextInterface> &context,
                                          const std::shared_ptr<TextureHolderInterface> &textureHolder) {
    if (tessellatedGraphicsObject) {
        tessellatedGraphicsObject->loadTexture(context, textureHolder);
    } else if (quadGraphicsObject) {
        quadGraphicsObject->loadTexture(context, textureHolder);
    }
}

void TexturedPolygonOpenGl::loadDualTexture(const std::shared_ptr<::RenderingContextInterface> &context,
                                              const std::shared_ptr<TextureHolderInterface> &textureHolder,
                                              const std::shared_ptr<TextureHolderInterface> &elevationHolder) {
    if (tessellatedGraphicsObject) {
        tessellatedGraphicsObject->loadDualTexture(context, textureHolder, elevationHolder);
    } else if (quadGraphicsObject) {
        quadGraphicsObject->loadDualTexture(context, textureHolder, elevationHolder);
    }
}

void TexturedPolygonOpenGl::removeTexture() {
    if (tessellatedGraphicsObject) {
        tessellatedGraphicsObject->removeTexture();
    } else if (quadGraphicsObject) {
        quadGraphicsObject->removeTexture();
    }
}

std::shared_ptr<GraphicsObjectInterface> TexturedPolygonOpenGl::asGraphicsObject() {
    if (tessellatedGraphicsObject) {
        return tessellatedGraphicsObject->asGraphicsObject();
    }
    return quadGraphicsObject->asGraphicsObject();
}

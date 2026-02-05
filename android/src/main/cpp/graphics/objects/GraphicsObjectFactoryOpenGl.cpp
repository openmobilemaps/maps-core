/*
 * Copyright (c) 2021 Ubique Innovation AG <https://www.ubique.ch>
 *
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this
 *  file, You can obtain one at https://mozilla.org/MPL/2.0/.
 *
 *  SPDX-License-Identifier: MPL-2.0
 */

#include "GraphicsObjectFactoryOpenGl.h"
#include "ColorShaderOpenGl.h"
#include "LineGroup2dOpenGl.h"
#include "Polygon2dOpenGl.h"
#include "PolygonGroup2dOpenGl.h"
#include "PolygonPatternGroup2dOpenGl.h"
#include "Quad2dInstancedOpenGl.h"
#include "Quad2dOpenGl.h"
#include "Text2dOpenGl.h"
#include "Text2dInstancedOpenGl.h"
#include "Quad2dStretchedInstancedOpenGl.h"
#include "IcosahedronOpenGl.h"
#ifdef HARDWARE_TESSELLATION_SUPPORTED
#include "Quad2dTessellatedOpenGl.h"
#include "Polygon2dTessellatedOpenGl.h"
#include "TessellatedColorShaderOpenGl.h"
#endif

std::shared_ptr<Quad2dInterface> GraphicsObjectFactoryOpenGl::createQuad(const std::shared_ptr<::ShaderProgramInterface> &shader) {
    return std::make_shared<Quad2dOpenGl>(enforceGlShader(shader));
}

std::shared_ptr<Quad2dInterface> GraphicsObjectFactoryOpenGl::createQuadTessellated(const std::shared_ptr<::ShaderProgramInterface> &shader) {
#ifdef HARDWARE_TESSELLATION_SUPPORTED
    return std::make_shared<Quad2dTessellatedOpenGl>(enforceGlShader(shader));
#else
    return nullptr;
#endif

}

std::shared_ptr<Polygon2dInterface>
GraphicsObjectFactoryOpenGl::createPolygon(const std::shared_ptr<::ShaderProgramInterface> &shader) {
    return std::make_shared<Polygon2dOpenGl>(enforceGlShader(shader));
}

std::shared_ptr<Polygon2dInterface>
GraphicsObjectFactoryOpenGl::createPolygonTessellated(const std::shared_ptr<::ShaderProgramInterface> &shader) {
#ifdef HARDWARE_TESSELLATION_SUPPORTED
    return std::make_shared<Polygon2dTessellatedOpenGl>(enforceGlShader(shader));
#else
    return nullptr;
#endif
}

std::shared_ptr<LineGroup2dInterface>
GraphicsObjectFactoryOpenGl::createLineGroup(const std::shared_ptr<::ShaderProgramInterface> &shader) {
    return std::make_shared<LineGroup2dOpenGl>(enforceGlShader(shader));
}

std::shared_ptr<PolygonGroup2dInterface>
GraphicsObjectFactoryOpenGl::createPolygonGroup(const std::shared_ptr<::ShaderProgramInterface> &shader) {
    return std::make_shared<PolygonGroup2dOpenGl>(enforceGlShader(shader));
}

std::shared_ptr<PolygonPatternGroup2dInterface>
GraphicsObjectFactoryOpenGl::createPolygonPatternGroup(const std::shared_ptr<::ShaderProgramInterface> &shader) {
    return std::make_shared<PolygonPatternGroup2dOpenGl>(enforceGlShader(shader));
}

std::shared_ptr<Quad2dInterface> GraphicsObjectFactoryOpenGl::createQuadMask(bool is3D) {
    return std::make_shared<Quad2dOpenGl>(std::make_shared<ColorShaderOpenGl>(is3D));
}

std::shared_ptr<Polygon2dInterface> GraphicsObjectFactoryOpenGl::createPolygonMask(bool is3D) {
    std::shared_ptr<ColorShaderOpenGl> shader = std::make_shared<ColorShaderOpenGl>(is3D);
    shader->setColor(1, 1, 1, 1);
    return std::make_shared<Polygon2dOpenGl>(enforceGlShader(shader));
}

std::shared_ptr<Polygon2dInterface> GraphicsObjectFactoryOpenGl::createPolygonMaskTessellated(bool is3D) {
#ifdef HARDWARE_TESSELLATION_SUPPORTED
    std::shared_ptr<TessellatedColorShaderOpenGl> shader = std::make_shared<TessellatedColorShaderOpenGl>(is3D);
    shader->setColor(1, 1, 1, 1);
    return std::make_shared<Polygon2dTessellatedOpenGl>(enforceGlShader(shader));
#else
    return nullptr;
#endif
}

std::shared_ptr<TextInterface> GraphicsObjectFactoryOpenGl::createText(const std::shared_ptr<::ShaderProgramInterface> &shader) {
    return std::make_shared<Text2dOpenGl>(enforceGlShader(shader));
}

std::shared_ptr<TextInstancedInterface> GraphicsObjectFactoryOpenGl::createTextInstanced(const std::shared_ptr<::ShaderProgramInterface> & shader) {
    return std::make_shared<Text2dInstancedOpenGl>(enforceGlShader(shader));
}

std::shared_ptr<Quad2dInstancedInterface> GraphicsObjectFactoryOpenGl::createQuadInstanced(const std::shared_ptr<::ShaderProgramInterface> &shader) {
    return std::make_shared<Quad2dInstancedOpenGl>(enforceGlShader(shader));
}

std::shared_ptr<Quad2dStretchedInstancedInterface>
GraphicsObjectFactoryOpenGl::createQuadStretchedInstanced(const std::shared_ptr<::ShaderProgramInterface> &shader) {
    return std::make_shared<Quad2dStretchedInstancedOpenGl>(enforceGlShader(shader));
}

std::shared_ptr<IcosahedronInterface>
GraphicsObjectFactoryOpenGl::createIcosahedronObject(const std::shared_ptr<::ShaderProgramInterface> &shader) {
    return std::make_shared<IcosahedronOpenGl>(enforceGlShader(shader));
}

std::shared_ptr<BaseShaderProgramOpenGl> GraphicsObjectFactoryOpenGl::enforceGlShader(const std::shared_ptr<::ShaderProgramInterface> &shader) {
    std::shared_ptr<BaseShaderProgramOpenGl> glShader = std::dynamic_pointer_cast<BaseShaderProgramOpenGl>(shader);
    if (!glShader) {
        throw std::runtime_error("GraphicsObjectFactoryOpenGl: ShaderProgramInterface doesn't extend BaseShaderProgramOpenGl!");
    }
    return glShader;
}

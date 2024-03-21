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

std::shared_ptr<Quad2dInterface> GraphicsObjectFactoryOpenGl::createQuad(const std::shared_ptr<::ShaderProgramInterface> &shader) {
    return std::make_shared<Quad2dOpenGl>(shader);
}

std::shared_ptr<Polygon2dInterface>
GraphicsObjectFactoryOpenGl::createPolygon(const std::shared_ptr<::ShaderProgramInterface> &shader) {
    return std::make_shared<Polygon2dOpenGl>(shader);
}

std::shared_ptr<LineGroup2dInterface>
GraphicsObjectFactoryOpenGl::createLineGroup(const std::shared_ptr<::ShaderProgramInterface> &shader) {
    return std::make_shared<LineGroup2dOpenGl>(shader);
}

std::shared_ptr<PolygonGroup2dInterface>
GraphicsObjectFactoryOpenGl::createPolygonGroup(const std::shared_ptr<::ShaderProgramInterface> &shader) {
    return std::make_shared<PolygonGroup2dOpenGl>(shader);
}

std::shared_ptr<PolygonPatternGroup2dInterface>
GraphicsObjectFactoryOpenGl::createPolygonPatternGroup(const std::shared_ptr<::ShaderProgramInterface> &shader) {
    return std::make_shared<PolygonPatternGroup2dOpenGl>(shader);
}

std::shared_ptr<Quad2dInterface> GraphicsObjectFactoryOpenGl::createQuadMask() {
    return std::make_shared<Quad2dOpenGl>(std::make_shared<ColorShaderOpenGl>(false));
}

std::shared_ptr<Polygon2dInterface> GraphicsObjectFactoryOpenGl::createPolygonMask() {
    std::shared_ptr<ColorShaderOpenGl> shader = std::make_shared<ColorShaderOpenGl>(false);
    shader->setColor(1, 1, 1, 1);
    return std::make_shared<Polygon2dOpenGl>(shader);
}

std::shared_ptr<TextInterface> GraphicsObjectFactoryOpenGl::createText(const std::shared_ptr<::ShaderProgramInterface> &shader) {
    return std::make_shared<Text2dOpenGl>(shader);
}

std::shared_ptr<TextInstancedInterface> GraphicsObjectFactoryOpenGl::createTextInstanced(const std::shared_ptr<::ShaderProgramInterface> & shader) {
    return std::make_shared<Text2dInstancedOpenGl>(shader);
}

std::shared_ptr<Quad2dInstancedInterface> GraphicsObjectFactoryOpenGl::createQuadInstanced(const std::shared_ptr<::ShaderProgramInterface> &shader) {
    return std::make_shared<Quad2dInstancedOpenGl>(shader);
}

std::shared_ptr<Quad2dStretchedInstancedInterface>
GraphicsObjectFactoryOpenGl::createQuadStretchedInstanced(const std::shared_ptr<::ShaderProgramInterface> &shader) {
    return std::make_shared<Quad2dStretchedInstancedOpenGl>(shader);
}

std::shared_ptr<IcosahedronInterface>
GraphicsObjectFactoryOpenGl::createIcosahedronObject(const std::shared_ptr<::ShaderProgramInterface> &shader) {
    return std::make_shared<IcosahedronOpenGl>(shader);
}
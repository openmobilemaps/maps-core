/*
 * Copyright (c) 2021 Ubique Innovation AG <https://www.ubique.ch>
 *
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this
 *  file, You can obtain one at https://mozilla.org/MPL/2.0/.
 *
 *  SPDX-License-Identifier: MPL-2.0
 */

#include "ShaderFactoryOpenGl.h"
#include "AlphaShaderOpenGl.h"
#include "ColorCircleShaderOpenGl.h"
#include "ColorLineGroup2dShaderOpenGl.h"
#include "ColorPolygonGroup2dShaderOpenGl.h"
#include "PolygonPatternGroup2dShaderOpenGl.h"
#include "ColorShaderOpenGl.h"
#include "TextShaderOpenGl.h"
#include "TextInstancedShaderOpenGl.h"
#include "RasterShaderOpenGl.h"
#include "StretchShaderOpenGl.h"
#include "AlphaInstancedShaderOpenGl.h"
#include "StretchInstancedShaderOpenGl.h"
#include "IcosahedronColorShaderOpenGl.h"

std::shared_ptr<AlphaShaderInterface> ShaderFactoryOpenGl::createAlphaShader() {
    return std::make_shared<AlphaShaderOpenGl>(false);
}

std::shared_ptr<AlphaShaderInterface> ShaderFactoryOpenGl::createUnitSphereAlphaShader() {
    return std::make_shared<AlphaShaderOpenGl>(true);
}

std::shared_ptr<AlphaInstancedShaderInterface> ShaderFactoryOpenGl::createAlphaInstancedShader() {
    return std::make_shared<AlphaInstancedShaderOpenGl>();
}

std::shared_ptr<RasterShaderInterface> ShaderFactoryOpenGl::createRasterShader() {
    return std::make_shared<RasterShaderOpenGl>(false);
}

std::shared_ptr<RasterShaderInterface> ShaderFactoryOpenGl::createUnitSphereRasterShader() {
    return std::make_shared<RasterShaderOpenGl>(true);
}

std::shared_ptr<LineGroupShaderInterface> ShaderFactoryOpenGl::createLineGroupShader() {
    return std::make_shared<ColorLineGroup2dShaderOpenGl>();
}

std::shared_ptr<ColorShaderInterface> ShaderFactoryOpenGl::createColorShader() {
    return std::make_shared<ColorShaderOpenGl>();
}

std::shared_ptr<ColorCircleShaderInterface> ShaderFactoryOpenGl::createColorCircleShader() {
    return std::make_shared<ColorCircleShaderOpenGl>();
}

std::shared_ptr<PolygonGroupShaderInterface> ShaderFactoryOpenGl::createPolygonGroupShader(bool isStriped) {
    return std::make_shared<ColorPolygonGroup2dShaderOpenGl>(isStriped);
}

std::shared_ptr<PolygonPatternGroupShaderInterface> ShaderFactoryOpenGl::createPolygonPatternGroupShader(bool fadeInPattern) {
    return std::make_shared<PolygonPatternGroup2dShaderOpenGl>(fadeInPattern);
}

std::shared_ptr<TextShaderInterface> ShaderFactoryOpenGl::createTextShader() {
    return std::make_shared<TextShaderOpenGl>();
}

std::shared_ptr<TextInstancedShaderInterface> ShaderFactoryOpenGl::createTextInstancedShader() {
    return std::make_shared<TextInstancedShaderOpenGl>();
}

std::shared_ptr<StretchShaderInterface> ShaderFactoryOpenGl::createStretchShader() {
    return std::make_shared<StretchShaderOpenGl>();
}

std::shared_ptr<StretchInstancedShaderInterface> ShaderFactoryOpenGl::createStretchInstancedShader() {
    return std::make_shared<StretchInstancedShaderOpenGl>();
}

std::shared_ptr<ColorShaderInterface> ShaderFactoryOpenGl::createIcosahedronColorShader() {
    return std::make_shared<IcosahedronColorShaderOpenGl>();
}

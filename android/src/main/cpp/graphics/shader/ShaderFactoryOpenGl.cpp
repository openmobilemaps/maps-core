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
#include "ColorLineShaderOpenGl.h"
#include "ColorPolygonGroup2dShaderOpenGl.h"
#include "ColorShaderOpenGl.h"
#include "TextShaderOpenGl.h"
#include "RasterShaderOpenGl.h"

std::shared_ptr<AlphaShaderInterface> ShaderFactoryOpenGl::createAlphaShader() {
    return std::make_shared<AlphaShaderOpenGl>();
}

std::shared_ptr<RasterShaderInterface> ShaderFactoryOpenGl::createRasterShader() {
    return std::make_shared<RasterShaderOpenGl>();
}

std::shared_ptr<ColorLineShaderInterface> ShaderFactoryOpenGl::createColorLineShader() {
    return std::make_shared<ColorLineShaderOpenGl>();
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

std::shared_ptr<PolygonGroupShaderInterface> ShaderFactoryOpenGl::createPolygonGroupShader() {
    return std::make_shared<ColorPolygonGroup2dShaderOpenGl>();
}

std::shared_ptr<TextShaderInterface> ShaderFactoryOpenGl::createTextShader() {
    return std::make_shared<TextShaderOpenGl>();
}

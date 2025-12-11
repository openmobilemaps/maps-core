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

#include "ShaderFactoryInterface.h"

class ShaderFactoryOpenGl : public ShaderFactoryInterface {

public:
    std::shared_ptr<AlphaShaderInterface> createAlphaShader() override;

    std::shared_ptr<AlphaShaderInterface> createUnitSphereAlphaShader() override;

    std::shared_ptr<AlphaInstancedShaderInterface> createAlphaInstancedShader() override;

    std::shared_ptr<LineGroupShaderInterface> createLineGroupShader() override;

    std::shared_ptr<LineGroupShaderInterface> createUnitSphereLineGroupShader() override;

    std::shared_ptr<LineGroupShaderInterface> createSimpleLineGroupShader() override;

    std::shared_ptr<LineGroupShaderInterface> createUnitSphereSimpleLineGroupShader() override;

    std::shared_ptr<ColorShaderInterface> createColorShader() override;

    std::shared_ptr<ColorShaderInterface> createUnitSphereColorShader() override;

    std::shared_ptr<ColorShaderInterface> createPolygonTessellatedShader(bool projectOntoUnitSphere) override;

    std::shared_ptr<ColorCircleShaderInterface> createColorCircleShader() override;

    std::shared_ptr<AlphaInstancedShaderInterface> createUnitSphereAlphaInstancedShader() override;

    std::shared_ptr<TextInstancedShaderInterface> createUnitSphereTextInstancedShader() override;

    std::shared_ptr<ColorCircleShaderInterface> createUnitSphereColorCircleShader() override;

    std::shared_ptr<PolygonGroupShaderInterface> createPolygonGroupShader(bool isStriped, bool unitSphere) override;

    std::shared_ptr<PolygonPatternGroupShaderInterface> createPolygonPatternGroupShader(bool fadeInPattern, bool unitSphere) override;

    std::shared_ptr<TextShaderInterface> createTextShader() override;

    std::shared_ptr<TextInstancedShaderInterface> createTextInstancedShader() override;

    std::shared_ptr<RasterShaderInterface> createRasterShader() override;

    std::shared_ptr<RasterShaderInterface> createUnitSphereRasterShader() override;

    std::shared_ptr<RasterShaderInterface> createQuadTessellatedShader() override;

    std::shared_ptr<StretchShaderInterface> createStretchShader() override;

    std::shared_ptr<StretchInstancedShaderInterface> createStretchInstancedShader(bool unitSphere) override;

    std::shared_ptr<ColorShaderInterface> createIcosahedronColorShader() override;

    std::shared_ptr<SphereEffectShaderInterface> createSphereEffectShader() override;

    std::shared_ptr<SkySphereShaderInterface> createSkySphereShader() override;

    std::shared_ptr<ElevationInterpolationShaderInterface> createElevationInterpolationShader() override;
};

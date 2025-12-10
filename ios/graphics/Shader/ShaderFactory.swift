/*
 * Copyright (c) 2021 Ubique Innovation AG <https://www.ubique.ch>
 *
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this
 *  file, You can obtain one at https://mozilla.org/MPL/2.0/.
 *
 *  SPDX-License-Identifier: MPL-2.0
 */

import Foundation
import MapCoreSharedModule

class ShaderFactory: MCShaderFactoryInterface {
    func createSkySphereShader() -> (any MCSkySphereShaderInterface)? {
        SkySphereShader()
    }

    func createStretchInstancedShader(_ unitSphere: Bool) -> (any MCStretchInstancedShaderInterface)? {
        StretchInstancedShader()
    }

    func createUnitSphereRasterShader() -> MCRasterShaderInterface? {
        RasterShader()
    }

    func createTextShader() -> MCTextShaderInterface? {
        TextShader()
    }

    func createPolygonGroupShader(_ isStriped: Bool, unitSphere: Bool) -> MCPolygonGroupShaderInterface? {
        PolygonGroupShader(isStriped: isStriped, isUnitSphere: unitSphere)
    }

    func createPolygonPatternGroupShader(_ fadeInPattern: Bool, unitSphere: Bool) -> MCPolygonPatternGroupShaderInterface? {
        PolygonPatternGroupShader(fadeInPattern: fadeInPattern, isUnitSphere: unitSphere)
    }

    func createColorCircleShader() -> MCColorCircleShaderInterface? {
        ColorCircleShader()
    }

    func createUnitSphereColorCircleShader() -> (any MCColorCircleShaderInterface)? {
        ColorCircleShader(shader: .unitSphereRoundColorShader)
    }

    func createAlphaShader() -> MCAlphaShaderInterface? {
        AlphaShader()
    }

    func createUnitSphereAlphaShader() -> MCAlphaShaderInterface? {
        AlphaShader(shader: .unitSphereAlphaShader)
    }

    func createAlphaInstancedShader() -> MCAlphaInstancedShaderInterface? {
        AlphaInstancedShader()
    }

    func createUnitSphereAlphaInstancedShader() -> (any MCAlphaInstancedShaderInterface)? {
        AlphaInstancedShader(shader: .unitSphereAlphaInstancedShader, unitSphere: true)
    }

    func createLineGroupShader() -> MCLineGroupShaderInterface? {
        LineGroupShader()
    }

    func createUnitSphereLineGroupShader() -> (any MCLineGroupShaderInterface)? {
        LineGroupShader(shader: .unitSphereLineGroupShader)
    }

    func createSimpleLineGroupShader() -> MCLineGroupShaderInterface? {
        LineGroupShader(shader: .simpleLineGroupShader)
    }

    func createUnitSphereSimpleLineGroupShader() -> (any MCLineGroupShaderInterface)? {
        LineGroupShader(shader: .unitSphereSimpleLineGroupShader)
    }

    func createUnitSphereColorShader() -> MCColorShaderInterface? {
        ColorShader()
    }

    func createColorShader() -> MCColorShaderInterface? {
        ColorShader()
    }

    func createRasterShader() -> MCRasterShaderInterface? {
        RasterShader()
    }

    func createStretchShader() -> MCStretchShaderInterface? {
        StretchShader()
    }

    func createTextInstancedShader() -> MCTextInstancedShaderInterface? {
        TextInstancedShader()
    }

    func createUnitSphereTextInstancedShader() -> (any MCTextInstancedShaderInterface)? {
        TextInstancedShader(unitSphere: true)
    }

    func createStretchInstancedShader() -> MCStretchInstancedShaderInterface? {
        StretchInstancedShader()
    }

    func createIcosahedronColorShader() -> MCColorShaderInterface? {
        IcosahedronShader()
    }

    func createSphereEffectShader() -> (any MCSphereEffectShaderInterface)? {
        SphereEffectShader()
    }

    func createElevationInterpolationShader() -> (any MCElevationInterpolationShaderInterface)? {
        ElevationInterpolationShader()
    }
}

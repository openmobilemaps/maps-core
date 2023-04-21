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
    func createTextShader() -> MCTextShaderInterface? {
        TextShader()
    }

    func createPolygonGroupShader() -> MCPolygonGroupShaderInterface? {
        PolygonGroupShader()
    }

    func createColorCircleShader() -> MCColorCircleShaderInterface? {
        ColorCircleShader()
    }

    func createAlphaShader() -> MCAlphaShaderInterface? {
        AlphaShader()
    }

    func createAlphaInstancedShader() -> MCAlphaInstancedShaderInterface? {
        AlphaInstancedShader()
    }

    func createColorLineShader() -> MCColorLineShaderInterface? {
        LineGroupShader(styleBufferSize: 1)
    }

    func createLineGroupShader() -> MCLineGroupShaderInterface? {
        LineGroupShader()
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
}

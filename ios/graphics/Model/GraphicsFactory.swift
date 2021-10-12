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

class GraphicsFactory: MCGraphicsObjectFactoryInterface {
    func createPolygonGroup(_ shader: MCShaderProgramInterface?) -> MCPolygonGroup2dInterface? {
        guard let shader = shader else { fatalError("No Shader provided") }
        return PolygonGroup2d(shader: shader, metalContext: .current)
    }

    func createQuad(_ shader: MCShaderProgramInterface?) -> MCQuad2dInterface? {
        guard let shader = shader else { fatalError("No Shader provided") }
        return Quad2d(shader: shader, metalContext: .current)
    }

    func createLine(_ shader: MCShaderProgramInterface?) -> MCLine2dInterface? {
        guard let shader = shader else { fatalError("No Shader provided") }
        return Line2d(shader: shader, metalContext: .current)
    }

    func createLineGroup(_ shader: MCShaderProgramInterface?) -> MCLineGroup2dInterface? {
        guard let shader = shader else { fatalError("No Shader provided") }
        return LineGroup2d(shader: shader, metalContext: .current)
    }

    func createPolygon(_ shader: MCShaderProgramInterface?) -> MCPolygon2dInterface? {
        guard let shader = shader else { fatalError("No Shader provided") }
        return Polygon2d(shader: shader, metalContext: .current)
    }

    func createQuadMask() -> MCQuad2dInterface? {
        Quad2d(shader: ColorShader(), metalContext: .current)
    }

    func createPolygonMask() -> MCPolygon2dInterface? {
        Polygon2d(shader: ColorShader(), metalContext: .current)
    }

    func createText(_ shader: MCShaderProgramInterface?) -> MCTextInterface? {
        guard let shader = shader else { fatalError("No Shader provided") }
        return Text(shader: shader, metalContext: .current)
    }
}

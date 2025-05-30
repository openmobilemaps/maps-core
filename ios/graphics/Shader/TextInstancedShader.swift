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
@preconcurrency import Metal

class TextInstancedShader: BaseShader, @unchecked Sendable {

    public let isUnitSphere: Bool

    init(unitSphere: Bool = false) {
        self.isUnitSphere = unitSphere
        super.init(shader: unitSphere ? .unitSphereTextInstancedShader : .textInstancedShader)
    }

    override func setupProgram(_: MCRenderingContextInterface?) {
        if pipeline == nil {
            pipeline = MetalContext.current.pipelineLibrary.value(Pipeline(type: shader, blendMode: blendMode))
        }
    }

    override func preRender(encoder: MTLRenderCommandEncoder, context: RenderingContext) {
        guard let pipeline else { return }
        context.setRenderPipelineStateIfNeeded(pipeline)
    }
}

extension TextInstancedShader: MCTextInstancedShaderInterface {
    func asShaderProgram() -> MCShaderProgramInterface? {
        self
    }
}

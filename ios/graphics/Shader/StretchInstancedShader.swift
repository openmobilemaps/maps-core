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
import Metal

class StretchInstancedShader: BaseShader {
    private var pipeline: MTLRenderPipelineState?

    private let shader : Pipeline

    init(shader : Pipeline = Pipeline.stretchInstancedShader) {
        self.shader = shader
    }

    override func setupProgram(_: MCRenderingContextInterface?) {
        if pipeline == nil {
            pipeline = MetalContext.current.pipelineLibrary.value(shader.rawValue)
        }
    }

    override func preRender(encoder: MTLRenderCommandEncoder, context: RenderingContext) {
        guard let pipeline = pipeline else { return }

        context.setRenderPipelineStateIfNeeded(pipeline)
    }
}

extension StretchInstancedShader: MCStretchInstancedShaderInterface {
    func asShaderProgram() -> MCShaderProgramInterface? {
        self
    }
}

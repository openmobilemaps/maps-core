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

class AlphaInstancedShader: BaseShader {
    private let shader: PipelineType

    init(shader: PipelineType = .alphaInstancedShader) {
        self.shader = shader
    }

    override func setupProgram(_: MCRenderingContextInterface?) {
        pipelineAccessQueue.sync {
            if pipeline == nil {
                pipeline = MetalContext.current.pipelineLibrary.value(Pipeline(type: shader, blendMode: blendMode).json)
            }
        }
    }

    override func preRender(encoder: MTLRenderCommandEncoder, context: RenderingContext) {
        guard let pipeline else { return }

        context.setRenderPipelineStateIfNeeded(pipeline)
    }
}

extension AlphaInstancedShader: MCAlphaInstancedShaderInterface {
    func asShaderProgram() -> MCShaderProgramInterface? {
        self
    }
}

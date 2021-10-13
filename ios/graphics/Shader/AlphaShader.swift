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

class AlphaShader: BaseShader {
    private var alpha: Float = 1.0
    private var pipeline: MTLRenderPipelineState?

    override func setupProgram(_: MCRenderingContextInterface?) {
        if pipeline == nil {
            pipeline = MetalContext.current.pipelineLibrary.value(PipelineKey.alphaShader)
        }
    }

    override func preRender(encoder: MTLRenderCommandEncoder, context _: RenderingContext) {
        guard let pipeline = pipeline else { return }

        encoder.setRenderPipelineState(pipeline)
        encoder.setFragmentBytes(&alpha, length: MemoryLayout<Float>.stride, index: 1)
    }
}

extension AlphaShader: MCAlphaShaderInterface {
    func updateAlpha(_ value: Float) {
        alpha = value
    }

    func asShaderProgram() -> MCShaderProgramInterface? {
        self
    }
}

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

class ColorShader: BaseShader {
    private var color = SIMD4<Float>([0.0, 0.0, 0.0, 0.0])

    private var stencilState: MTLDepthStencilState?

    private let shader: PipelineType

    init(shader: PipelineType = .colorShader) {
        self.shader = shader
    }

    override func setupProgram(_: MCRenderingContextInterface?) {
        if pipeline == nil {
            pipeline = MetalContext.current.pipelineLibrary.value(Pipeline(type: shader, blendMode: blendMode).json)
        }
    }

    override func preRender(encoder: MTLRenderCommandEncoder, context: RenderingContext) {
        guard let pipeline else { return }
        context.setRenderPipelineStateIfNeeded(pipeline)
        encoder.setFragmentBytes(&color, length: MemoryLayout<SIMD4<Float>>.stride, index: 1)
    }
}

extension ColorShader: MCColorShaderInterface {
    func setColor(_ red: Float, green: Float, blue: Float, alpha: Float) {
        color = [red, green, blue, alpha]
    }

    func asShaderProgram() -> MCShaderProgramInterface? {
        self
    }
}

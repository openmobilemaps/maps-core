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

class TextShader: BaseShader {
    private var opacity: Float = 1.0
    private var color = SIMD4<Float>([0.0, 0.0, 0.0, 0.0])
    private var haloColor = SIMD4<Float>([0.0, 0.0, 0.0, 0.0])
    private var haloWidth: Float = 0.0

    override func setupProgram(_: MCRenderingContextInterface?) {
        if pipeline == nil {
            pipeline = MetalContext.current.pipelineLibrary.value(Pipeline(type: .textShader, blendMode: blendMode).json)
        }
    }

    override func preRender(encoder: MTLRenderCommandEncoder, context: RenderingContext) {
        guard let pipeline else { return }

        context.setRenderPipelineStateIfNeeded(pipeline)

        encoder.setFragmentBytes(&color, length: MemoryLayout<SIMD4<Float>>.stride, index: 1)

        encoder.setFragmentBytes(&haloColor, length: MemoryLayout<SIMD4<Float>>.stride, index: 2)

        encoder.setFragmentBytes(&opacity, length: MemoryLayout<Float>.stride, index: 3)
        encoder.setFragmentBytes(&haloWidth, length: MemoryLayout<Float>.stride, index: 4)
    }
}

extension TextShader: MCTextShaderInterface {
    func setHaloColor(_ color: MCColor, width: Float, blur: Float) {
        self.haloColor = color.simdValues
        self.haloWidth = Float(width)
    }

    func setOpacity(_ opacity: Float) {
        self.opacity = opacity
    }

    func setColor(_ color: MCColor) {
        self.color = color.simdValues
    }

    func asShaderProgram() -> MCShaderProgramInterface? {
        self
    }
}

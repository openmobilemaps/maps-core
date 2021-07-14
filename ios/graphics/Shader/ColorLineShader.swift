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

class ColorLineShader: BaseShader {
    private var color = SIMD4<Float>([0.0, 0.0, 0.0, 0.0])

    private var style: MCLineStyle?

    private var pipeline: MTLRenderPipelineState?

    var screenPixelAsRealMeterFactor: Double = 1.0

}

extension ColorLineShader: MCLineShaderProgramInterface {
    func preRenderPoint(_ context: MCRenderingContextInterface?) {}

    func setupPointProgram(_: MCRenderingContextInterface?) {}

    func getRectProgramName() -> String { "" }

    func getPointProgramName() -> String { "" }

    func setupRectProgram(_: MCRenderingContextInterface?) {
        if pipeline == nil {
            pipeline = MetalContext.current.pipelineLibrary.value(PipelineKey.lineShader)
        }
    }


    func preRenderRect(_ context: MCRenderingContextInterface?) {
        guard let context = context as? RenderingContext,
              let encoder = context.encoder,
              let pipeline = pipeline,
              let style = style else { return }

        encoder.setRenderPipelineState(pipeline)

        var c = SIMD4<Float>(style.color.normal.simdValues)
        encoder.setFragmentBytes(&c, length: MemoryLayout<SIMD4<Float>>.stride, index: 1)

        var scaledWidth = style.width;

        if style.widthType == .SCREEN_PIXEL {
            scaledWidth *= Float(screenPixelAsRealMeterFactor)
        }

        var radius = scaledWidth / 2

        encoder.setFragmentBytes(&radius, length: MemoryLayout<Float>.stride, index: 2)

        encoder.setVertexBytes(&scaledWidth, length: MemoryLayout<Float>.stride, index: 2)

    }
}

extension ColorLineShader: MCColorLineShaderInterface {
    func setStyle(_ lineStyle: MCLineStyle) {
        style = lineStyle
    }

    func asLineShaderProgram() -> MCLineShaderProgramInterface? {
        self
    }
}

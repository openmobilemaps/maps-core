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
import simd

class ColorCircleShader: BaseShader, @unchecked Sendable {
    private var fillColor = SIMD4<Float>([0.0, 0.0, 0.0, 0.0])
    private var strokeColor = SIMD4<Float>([0.0, 0.0, 0.0, 0.0])
    private var innerRadius: Float = 1.0

    override init(shader: PipelineType = .roundColorShader) {
        super.init(shader: shader)
    }

    override func setupProgram(_: MCRenderingContextInterface?) {
        if pipeline == nil {
            pipeline = MetalContext.current.pipelineLibrary.value(Pipeline(type: shader, blendMode: blendMode))
        }
    }

    override func preRender(encoder: MTLRenderCommandEncoder, context: RenderingContext) {
        guard let pipeline = activePipeline else { return }
        context.setRenderPipelineStateIfNeeded(pipeline)
        let viewportSize = context.getViewportSize()
        var viewport = SIMD2<Float>(Float(viewportSize.x), Float(viewportSize.y))
        encoder.setVertexBytes(&viewport, length: MemoryLayout<SIMD2<Float>>.stride, index: 4)
        encoder.setFragmentBytes(&fillColor, length: MemoryLayout<SIMD4<Float>>.stride, index: 1)
        encoder.setFragmentBytes(&strokeColor, length: MemoryLayout<SIMD4<Float>>.stride, index: 2)
        encoder.setFragmentBytes(&innerRadius, length: MemoryLayout<Float>.stride, index: 3)
    }
}

extension ColorCircleShader: MCColorCircleShaderInterface {
    func setColor(_ red: Float, green: Float, blue: Float, alpha: Float) {
        fillColor = [red, green, blue, alpha]
    }

    func setCircleStyle(
        _ fillRed: Float, fillGreen: Float, fillBlue: Float, fillAlpha: Float,
        strokeRed: Float, strokeGreen: Float, strokeBlue: Float, strokeAlpha: Float,
        innerRadius: Float
    ) {
        fillColor = [fillRed, fillGreen, fillBlue, fillAlpha]
        strokeColor = [strokeRed, strokeGreen, strokeBlue, strokeAlpha]
        self.innerRadius = min(1.0, max(0.0, innerRadius))
    }

    func asShaderProgram() -> MCShaderProgramInterface? {
        self
    }
}

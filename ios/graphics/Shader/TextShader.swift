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
    private var pipeline: MTLRenderPipelineState?
    private var referencePoint = MCVec3D()
    private var scaleFactor: Float = 1.0
    private var color = MCColor(r: 0.0, g: 0.0, b: 0.0, a: 0.0)

    override func setupProgram(_: MCRenderingContextInterface?) {
        if pipeline == nil {
            pipeline = MetalContext.current.pipelineLibrary.value(Pipeline.textShader.rawValue)
        }
    }

    override func preRender(encoder: MTLRenderCommandEncoder, context _: RenderingContext) {
        guard let pipeline = pipeline else { return }

        encoder.setRenderPipelineState(pipeline)

        var reference = SIMD2<Float>([referencePoint.x, referencePoint.y])
        encoder.setVertexBytes(&reference, length: MemoryLayout<SIMD2<Float>>.stride, index: 3)

        encoder.setVertexBytes(&scaleFactor, length: MemoryLayout<Float>.stride, index: 2)

        var c = color.simdValues
        encoder.setFragmentBytes(&c, length: MemoryLayout<SIMD4<Float>>.stride, index: 1)
    }
}

extension TextShader: MCTextShaderInterface {
    func setReferencePoint(_ point: MCVec3D) {
        referencePoint = point
    }

    func setScale(_ scale: Float) {
        scaleFactor = scale
    }

    func setColor(_ red: Float, green: Float, blue: Float, alpha: Float) {
        color = MCColor(r: red, g: green, b: blue, a: alpha)
    }

    func asShaderProgram() -> MCShaderProgramInterface? {
        self
    }
}

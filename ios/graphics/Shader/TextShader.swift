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
    private var referencePoint = SIMD2<Float>([0.0, 0.0])
    private var scaleFactor: Float = 1.0
    private var color = SIMD4<Float>([0.0, 0.0, 0.0, 0.0])
    private var haloColor = SIMD4<Float>([0.0, 0.0, 0.0, 0.0])

    override func setupProgram(_: MCRenderingContextInterface?) {
        if pipeline == nil {
            pipeline = MetalContext.current.pipelineLibrary.value(Pipeline.textShader.rawValue)
        }
    }

    override func preRender(encoder: MTLRenderCommandEncoder, context _: RenderingContext) {
        guard let pipeline = pipeline else { return }

        encoder.setRenderPipelineState(pipeline)

        encoder.setVertexBytes(&referencePoint, length: MemoryLayout<SIMD2<Float>>.stride, index: 3)

        encoder.setVertexBytes(&scaleFactor, length: MemoryLayout<Float>.stride, index: 2)

        encoder.setFragmentBytes(&color, length: MemoryLayout<SIMD4<Float>>.stride, index: 1)

        encoder.setFragmentBytes(&haloColor, length: MemoryLayout<SIMD4<Float>>.stride, index: 2)
    }
}

extension TextShader: MCTextShaderInterface {
    func setReferencePoint(_ point: MCVec3D) {
        referencePoint = SIMD2<Float>([point.x, point.y])
    }

    func setScale(_ scale: Float) {
        scaleFactor = scale
    }

    func setColor(_ color: MCColor) {
        self.color = color.simdValues
    }

    func setHaloColor(_ color: MCColor) {
        self.haloColor = color.simdValues
    }

    func asShaderProgram() -> MCShaderProgramInterface? {
        self
    }
}

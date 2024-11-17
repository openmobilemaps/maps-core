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
import UIKit
import simd

class LineGroupShader: BaseShader, @unchecked Sendable {
    var lineStyleBuffer: MTLBuffer?
    private var screenPixelAsRealMeterFactorBuffers: MultiBuffer<simd_float1>
    private var dashingScaleFactorBuffers: MultiBuffer<simd_float1>

    var screenPixelAsRealMeterFactor: Float = 1.0
    var lastScreenPixelAsRealMeterFactor: Float?
    var lastScreenPixelAsRealMeterFactorBuffer: MTLBuffer?

    var dashingScaleFactor: Float = 1.0
    var lastDashingScaleFactor: Float?
    var lastDashingScaleFactorBuffer: MTLBuffer?

    private let shaderType: PipelineType

    init(shader: PipelineType = .lineGroupShader) {
        self.shaderType = shader
        self.screenPixelAsRealMeterFactorBuffers = .init(device: MetalContext.current.device)
        self.dashingScaleFactorBuffers = .init(device: MetalContext.current.device)
    }

    override func setupProgram(_: MCRenderingContextInterface?) {
        if pipeline == nil {
            pipeline = MetalContext.current.pipelineLibrary.value(Pipeline(type: shaderType, blendMode: blendMode).json)
        }
    }

    override func preRender(encoder: MTLRenderCommandEncoder, context: RenderingContext) {
        guard let pipeline else { return }

        context.setRenderPipelineStateIfNeeded(pipeline)

        if lastScreenPixelAsRealMeterFactor != screenPixelAsRealMeterFactor || lastScreenPixelAsRealMeterFactorBuffer == nil {
            let screenPixelAsRealMeterFactorBuffer = screenPixelAsRealMeterFactorBuffers.getNextBuffer(context)
            if let bufferPointer = screenPixelAsRealMeterFactorBuffer?.contents()
                .assumingMemoryBound(to: simd_float1.self)
            {
                bufferPointer.pointee = screenPixelAsRealMeterFactor
            }
            lastScreenPixelAsRealMeterFactorBuffer = screenPixelAsRealMeterFactorBuffer
            lastScreenPixelAsRealMeterFactor = screenPixelAsRealMeterFactor
        }
        encoder.setVertexBuffer(lastScreenPixelAsRealMeterFactorBuffer!, offset: 0, index: 2)

        if lastDashingScaleFactor != dashingScaleFactor || lastDashingScaleFactorBuffer == nil {
            let dashingScaleFactorBuffer = dashingScaleFactorBuffers.getNextBuffer(context)
            if let bufferPointer = dashingScaleFactorBuffer?.contents()
                .assumingMemoryBound(to: simd_float1.self)
            {
                bufferPointer.pointee = dashingScaleFactor
            }
            lastDashingScaleFactorBuffer = dashingScaleFactorBuffer
            lastDashingScaleFactor = dashingScaleFactor
        }
        encoder.setVertexBuffer(lastDashingScaleFactorBuffer, offset: 0, index: 3)

        encoder.setVertexBuffer(lineStyleBuffer, offset: 0, index: 4)

        encoder.setFragmentBuffer(lineStyleBuffer, offset: 0, index: 1)
    }
}

extension LineGroupShader: MCLineGroupShaderInterface {
    func setStyles(_ styles: MCSharedBytes) {
        lineStyleBuffer.copyOrCreate(from: styles, device: MetalContext.current.device)
    }

    func setDashingScaleFactor(_ factor: Float) {
        dashingScaleFactor = factor
    }

    func asShaderProgram() -> MCShaderProgramInterface? {
        self
    }
}

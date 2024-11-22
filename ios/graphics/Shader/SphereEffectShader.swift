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

class SphereEffectShader: BaseShader, @unchecked Sendable {

    private var ellipse = [Float](repeating: 0.0, count: 16)

    private var ellipseBuffers: MultiBuffer<simd_float4x4>?

    private var stencilState: MTLDepthStencilState?

    init() {
        super.init(shader: .sphereEffectShader)
    }

    override func setupProgram(_: MCRenderingContextInterface?) {
        if pipeline == nil {
            pipeline = MetalContext.current.pipelineLibrary.value(Pipeline(type: shader, blendMode: blendMode))
            ellipseBuffers = .init(device: MetalContext.current.device)
        }
    }

    override func preRender(encoder: MTLRenderCommandEncoder, context: RenderingContext) {
        guard let pipeline else { return }
        context.setRenderPipelineStateIfNeeded(pipeline)

        if let ellipseBuffer = ellipseBuffers?.getNextBuffer(context) {
            ellipseBuffer.copyMemory(bytes: &ellipse, length: MemoryLayout<Float>.size * ellipse.count)
            encoder.setFragmentBuffer(ellipseBuffer, offset: 0, index: 0)
        }
    }
}

extension SphereEffectShader: MCSphereEffectShaderInterface {
    func asShaderProgram() -> (any MCShaderProgramInterface)? {
        return self
    }

    func setEllipse(_ coefficients: MCSharedBytes) {
        guard let rawPointer = UnsafeRawPointer(bitPattern: Int(coefficients.address)) else {
            return
        }

        let count : Int = Int(coefficients.elementCount)
        ellipse = Array(UnsafeBufferPointer(start: rawPointer.bindMemory(to: Float.self, capacity: count), count: count))
    }
}

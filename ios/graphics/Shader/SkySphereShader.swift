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

class SkySphereShader: BaseShader, @unchecked Sendable {

    private var inverseVP = [Float](repeating: 0.0, count: 16)
    private var cameraPosition = [Float](repeating: 0.0, count: 4)

    private var inverseVPBuffer: MultiBuffer<simd_float4x4>?
    private var cameraPositionBuffer: MultiBuffer<simd_float4>?

    private var stencilState: MTLDepthStencilState?

    init() {
        super.init(shader: .skySphereShader)
    }

    override func setupProgram(_: MCRenderingContextInterface?) {
        if pipeline == nil {
            pipeline = MetalContext.current.pipelineLibrary.value(Pipeline(type: shader, blendMode: blendMode))
            inverseVPBuffer = .init(device: MetalContext.current.device)
            cameraPositionBuffer = .init(device: MetalContext.current.device)
        }
    }

    override func preRender(encoder: MTLRenderCommandEncoder, context: RenderingContext) {
        guard let pipeline else { return }
        context.setRenderPipelineStateIfNeeded(pipeline)

        if let buffer = cameraPositionBuffer?.getNextBuffer(context) {
            buffer.copyMemory(bytes: &cameraPosition, length: MemoryLayout<Float>.size * cameraPosition.count)
            encoder.setFragmentBuffer(buffer, offset: 0, index: 1)
        }

        if let buffer = inverseVPBuffer?.getNextBuffer(context) {
            buffer.copyMemory(bytes: &inverseVP, length: MemoryLayout<Float>.size * inverseVP.count)
            encoder.setFragmentBuffer(buffer, offset: 0, index: 2)
        }
    }
}

extension SkySphereShader: MCSkySphereShaderInterface {
    func asShaderProgram() -> (any MCShaderProgramInterface)? {
        return self
    }

    func setCameraProperties(_ inverseVP: [NSNumber], cameraPosition: MCVec3D) {
        self.cameraPosition[0] = cameraPosition.xF
        self.cameraPosition[1] = cameraPosition.yF
        self.cameraPosition[2] = cameraPosition.zF
        self.cameraPosition[3] = 1.0

        for i in 0..<16 {
            self.inverseVP[i] = inverseVP[i].floatValue
        }
    }
}

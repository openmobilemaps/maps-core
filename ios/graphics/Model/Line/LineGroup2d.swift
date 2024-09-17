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

final class LineGroup2d: BaseGraphicsObject {
    private var shader: LineGroupShader

    private var lineVerticesBuffer: MTLBuffer?
    private var lineIndicesBuffer: MTLBuffer?
    private var indicesCount: Int = 0

    private var stencilState: MTLDepthStencilState?
    private var maskedStencilState: MTLDepthStencilState?

    private var customScreenPixelFactor: Float = 0

    init(shader: MCShaderProgramInterface, metalContext: MetalContext) {
        guard let shader = shader as? LineGroupShader else {
            fatalError("LineGroup2d only supports LineGroupShader")
        }
        self.shader = shader
        super.init(device: metalContext.device,
                   sampler: metalContext.samplerLibrary.value(Sampler.magLinear.rawValue)!,
                   label: "LineGroup2d")
    }

    private func setupStencilBufferDescriptor() {
        let ss = MTLStencilDescriptor()
        ss.stencilCompareFunction = .equal
        ss.stencilFailureOperation = .keep
        ss.depthFailureOperation = .keep
        ss.depthStencilPassOperation = .incrementClamp
        ss.writeMask = 0b0111_1111
        ss.readMask = 0b1111_1111

        let s = MTLDepthStencilDescriptor()
        s.frontFaceStencil = ss
        s.backFaceStencil = ss
        s.label = "LineGroup2d.maskedStencilState"

        maskedStencilState = MetalContext.current.device.makeDepthStencilState(descriptor: s)

        let mss = MTLStencilDescriptor()
        mss.stencilCompareFunction = .notEqual
        mss.stencilFailureOperation = .keep
        mss.depthFailureOperation = .keep
        mss.depthStencilPassOperation = .replace
        ss.writeMask = 0xFF

        let ms = MTLDepthStencilDescriptor()
        ms.frontFaceStencil = mss
        ms.backFaceStencil = mss
        ms.label = "LineGroup2d.stencilState"

        stencilState = MetalContext.current.device.makeDepthStencilState(descriptor: ms)
    }

    override func render(encoder: MTLRenderCommandEncoder,
                         context: RenderingContext,
                         renderPass _: MCRenderPassConfig,
                         mvpMatrix: Int64,
                         isMasked: Bool,
                         screenPixelAsRealMeterFactor: Double) {
        lock.lock()
        defer {
            lock.unlock()
        }

        guard let lineVerticesBuffer,
              let lineIndicesBuffer,
              shader.lineStyleBuffer != nil
        else { return }

#if DEBUG
        encoder.pushDebugGroup(label)
        defer {
            encoder.popDebugGroup()
        }
#endif

        if stencilState == nil {
            setupStencilBufferDescriptor()
        }

        // draw call

        if isMasked {
            encoder.setDepthStencilState(maskedStencilState)
            encoder.setStencilReferenceValue(0b1100_0000)
        } else {
            encoder.setDepthStencilState(stencilState)
            encoder.setStencilReferenceValue(0xFF)
        }

        shader.screenPixelAsRealMeterFactor = Float(screenPixelAsRealMeterFactor)

        shader.setupProgram(context)
        shader.preRender(context)

        encoder.setVertexBuffer(lineVerticesBuffer, offset: 0, index: 0)
        let matrixPointer = UnsafeRawPointer(bitPattern: Int(mvpMatrix))!
        encoder.setVertexBytes(matrixPointer, length: 64, index: 1)

        encoder.drawIndexedPrimitives(type: .triangle,
                                      indexCount: indicesCount,
                                      indexType: .uint32,
                                      indexBuffer: lineIndicesBuffer,
                                      indexBufferOffset: 0)

        if !isMasked {
            context.clearStencilBuffer()
        }
    }
}

extension LineGroup2d: MCLineGroup2dInterface {
    func setLines(_ lines: MCSharedBytes, indices: MCSharedBytes) {
        guard lines.elementCount != 0 else {
            lock.withCritical {
                lineVerticesBuffer = nil
                lineIndicesBuffer = nil
                indicesCount = 0
            }

            return
        }
        lock.withCritical {
            self.lineVerticesBuffer.copyOrCreate(from: lines, device: device)
            self.lineIndicesBuffer.copyOrCreate(from: indices, device: device)
            if self.lineVerticesBuffer != nil, self.lineIndicesBuffer != nil {
                self.lineVerticesBuffer?.label = "LineGroup2d.verticesBuffer"
                self.lineIndicesBuffer?.label = "LineGroup2d.indicesBuffer"
                self.indicesCount = Int(indices.elementCount)
            } else {
                self.indicesCount = 0
            }
        }
    }

    func setScalingFactor(_ factor: Float) {
        lock.withCritical {
            customScreenPixelFactor = factor
        }
    }

    func asGraphicsObject() -> MCGraphicsObjectInterface? {
        self
    }
}

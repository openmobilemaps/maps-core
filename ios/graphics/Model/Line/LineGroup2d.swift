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

class LineGroup2d: BaseGraphicsObject {
    private var shader: LineGroupShader

    private var lineVerticesBuffer: MTLBuffer?
    private var lineIndicesBuffer: MTLBuffer?
    private var indicesCount: Int = 0

    private var stencilState: MTLDepthStencilState?
    private var maskedStencilState: MTLDepthStencilState?

    init(shader: MCShaderProgramInterface, metalContext: MetalContext) {
        guard let shader = shader as? LineGroupShader else {
            fatalError("LineGroup2d only supports LineGroupShader")
        }
        self.shader = shader
        super.init(device: metalContext.device,
                   sampler: metalContext.samplerLibrary.value(Sampler.magLinear.rawValue))
    }

    private func setupStencilBufferDescriptor() {
        let ss = MTLStencilDescriptor()
        ss.stencilCompareFunction = .equal
        ss.stencilFailureOperation = .keep
        ss.depthFailureOperation = .keep
        ss.depthStencilPassOperation = .incrementClamp
        ss.writeMask = 0b01111111
        ss.readMask =  0b11111111

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
        guard let lineVerticesBuffer = lineVerticesBuffer,
              let lineIndicesBuffer = lineIndicesBuffer
        else { return }

        //encoder.pushDebugGroup("LineGroup2d")

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

        //encoder.popDebugGroup()
    }
}

extension LineGroup2d: MCLineGroup2dInterface {
    func setLines(_ lines: MCSharedBytes, indices: MCSharedBytes) {
        guard lines.elementCount != 0 else {
            lineVerticesBuffer = nil
            lineIndicesBuffer = nil
            indicesCount = 0
            return
        }
        guard let verticesBuffer = device.makeBuffer(from: lines),
              let indicesBuffer = device.makeBuffer(from: indices)
        else {
            fatalError("Cannot allocate buffers for LineGroup2d")
        }

        verticesBuffer.label = "LineGroup2d.verticesBuffer"
        indicesBuffer.label = "LineGroup2d.indicesBuffer"

        indicesCount = Int(indices.elementCount)
        lineVerticesBuffer = verticesBuffer
        lineIndicesBuffer = indicesBuffer
    }

    func asGraphicsObject() -> MCGraphicsObjectInterface? {
        self
    }
}

extension LineGroup2d: MCLine2dInterface {
    func setLine(_ line: MCSharedBytes, indices: MCSharedBytes) {
        setLines(line, indices: indices)
    }
}

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
        ss.stencilCompareFunction = .less
        ss.stencilFailureOperation = .keep
        ss.depthFailureOperation = .keep
        ss.depthStencilPassOperation = .invert
        ss.writeMask = 0b0111_1111

        let s = MTLDepthStencilDescriptor()
        s.frontFaceStencil = ss
        s.backFaceStencil = ss

        maskedStencilState = MetalContext.current.device.makeDepthStencilState(descriptor: s)

        let mss = MTLStencilDescriptor()
        mss.stencilCompareFunction = .notEqual
        mss.stencilFailureOperation = .keep
        mss.depthFailureOperation = .keep
        mss.depthStencilPassOperation = .invert
        ss.writeMask = 0xFF

        let ms = MTLDepthStencilDescriptor()
        ms.frontFaceStencil = mss
        ms.backFaceStencil = mss

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

        encoder.pushDebugGroup("LineGroup2d")

        if stencilState == nil {
            setupStencilBufferDescriptor()
        }

        // draw call

        if isMasked {
            encoder.setDepthStencilState(maskedStencilState)
            encoder.setStencilReferenceValue(0b0000_0001)
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

        encoder.popDebugGroup()
    }
}

extension LineGroup2d: MCLineGroup2dInterface {
    func setLines(_ lines: [MCRenderLineDescription]) {
        guard lines.count >= 1 else {
            indicesCount = 0
            lineVerticesBuffer = nil
            lineIndicesBuffer = nil
            return
        }

        var lineVertices: [LineVertex] = Array()
        var indices: [UInt32] = []

        var sum = 0
        for line in lines {
            sum = sum + (line.positions.count - 1) * 4
        }

        lineVertices.reserveCapacity(sum)
        indices.reserveCapacity(sum * 3 / 2)

        var offset = 0

        var lineVertex = LineVertex(x: 0.0, y: 0.0, lineA: MCVec2D(x: 0.0, y: 0.0), lineB: MCVec2D(x: 0.0, y: 0.0), widthNormal: (x: 0.0, y: 0.0), lenghtNormal: (x: 0.0, y: 0.0))

        for line in lines {
            var lenghtPrefix: Float = 0.0

            var ci = line.positions[0]

            var i = 0

            while i < (line.positions.count - 1) {
                let iNext = i + 1
                let ciNext = line.positions[iNext]

                let lineNormalX = -(ciNext.yF - ci.yF)
                let lineNormalY = ciNext.xF - ci.xF
                let lineLength = sqrt(lineNormalX * lineNormalX + lineNormalY * lineNormalY)

                let miterX: Float = lineNormalX
                let miterY: Float = lineNormalY

                let ciX = ci.xF - (ciNext.xF - ci.xF)
                let ciY = ci.yF - (ciNext.yF - ci.yF)

                let ciNextX = ciNext.xF - (ci.xF - ciNext.xF)
                let ciNextY = ciNext.yF - (ci.yF - ciNext.yF)

                let fromOrigin = SIMD2<Float>(x: Float(ciNext.x - ci.x), y: Float(ciNext.y - ci.y))
                let divisor = sqrt(fromOrigin.x * fromOrigin.x + fromOrigin.y * fromOrigin.y)
                let unitVector = SIMD2<Float>(x: fromOrigin.x / divisor, y: fromOrigin.y / divisor)

                let segmentType: LineVertex.SegmantType
                if i == 0, i == (line.positions.count - 2) {
                    segmentType = .singleSegment
                } else if i == 0 {
                    segmentType = .start
                } else if i == (line.positions.count - 2) {
                    segmentType = .end
                } else {
                    segmentType = .inner
                }

                lineVertex.position = SIMD2<Float>(x: ciX - miterX, y: ciY - miterY)
                lineVertex.lineA = SIMD2<Float>(x: ci.xF, y: ci.yF)
                lineVertex.lineB = SIMD2<Float>(x: ciNext.xF, y: ciNext.yF)
                lineVertex.widthNormal = SIMD2<Float>(x: -lineNormalX / lineLength, y: -lineNormalY / lineLength)
                lineVertex.lenghtNormal = SIMD2<Float>(x: -unitVector.x, y: -unitVector.y)
                lineVertex.stylingIndex = line.styleIndex
                lineVertex.segmentType = segmentType.rawValue
                lineVertex.lenghtPrefix = lenghtPrefix

                lineVertices.append(lineVertex)

                lineVertex.position = SIMD2<Float>(x: ciX + miterX, y: ciY + miterY)
                lineVertex.widthNormal = SIMD2<Float>(x: lineNormalX / lineLength, y: lineNormalY / lineLength)

                lineVertices.append(lineVertex)

                lineVertex.position = SIMD2<Float>(x: ciNextX + miterX, y: ciNextY + miterY)
                lineVertex.lenghtNormal = unitVector

                lineVertices.append(lineVertex)

                lineVertex.position = SIMD2<Float>(x: ciNextX - miterX, y: ciNextY - miterY)
                lineVertex.widthNormal = SIMD2<Float>(x: -lineNormalX / lineLength, y: -lineNormalY / lineLength)

                lineVertices.append(lineVertex)

                let first = UInt32(4 * i + offset)
                indices.append(first)
                indices.append(first + 1)
                indices.append(first + 2)
                indices.append(first)
                indices.append(first + 2)
                indices.append(first + 3)

                lenghtPrefix += lineLength

                ci = ciNext

                i += 1
            }
            offset = lineVertices.count
        }

        guard let verticesBuffer = device.makeBuffer(bytes: lineVertices, length: MemoryLayout<LineVertex>.stride * lineVertices.count, options: []),
              let indicesBuffer = device.makeBuffer(bytes: indices, length: MemoryLayout<UInt32>.stride * indices.count, options: [])
        else {
            fatalError("Cannot allocate buffers for the UBTileModel")
        }

        indicesCount = indices.count
        lineVerticesBuffer = verticesBuffer
        lineIndicesBuffer = indicesBuffer
    }

    func asGraphicsObject() -> MCGraphicsObjectInterface? {
        self
    }
}

extension LineGroup2d: MCLine2dInterface {
    func setLinePositions(_ positions: [MCVec2D]) {
        setLines([
            MCRenderLineDescription(positions: positions, styleIndex: 0),
        ])
    }
}

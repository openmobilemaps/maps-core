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

class Line2d: BaseGraphicsObject {
    private var shader: MCLineShaderProgramInterface

    private var lineVerticesBuffer: MTLBuffer?
    private var lineIndicesBuffer: MTLBuffer?
    private var indicesCount: Int = 0

    private var stencilState: MTLDepthStencilState?
    private var clearStencilState: MTLDepthStencilState?

    init(shader: MCLineShaderProgramInterface, metalContext: MetalContext) {
        self.shader = shader
        super.init(device: metalContext.device,
                   sampler: metalContext.samplerLibrary.value(.magLinear))
    }

    private func setupStencilBufferDescriptor() {
        let ss = MTLStencilDescriptor()
        ss.stencilCompareFunction = .notEqual
        ss.stencilFailureOperation = .keep
        ss.depthFailureOperation = .keep
        ss.depthStencilPassOperation = .invert

        let s = MTLDepthStencilDescriptor()
        s.frontFaceStencil = ss
        s.backFaceStencil = ss

        stencilState = MetalContext.current.device.makeDepthStencilState(descriptor: s)

        let ss2 = MTLStencilDescriptor()
        ss2.stencilCompareFunction = .always
        ss2.stencilFailureOperation = .zero
        ss2.depthFailureOperation = .zero
        ss2.depthStencilPassOperation = .zero

        let s2 = MTLDepthStencilDescriptor()
        s2.frontFaceStencil = ss2
        s2.backFaceStencil = ss2

        clearStencilState = MetalContext.current.device.makeDepthStencilState(descriptor: s2)
    }

    override func render(encoder: MTLRenderCommandEncoder,
                         context: RenderingContext,
                         renderPass _: MCRenderPassConfig,
                         mvpMatrix: Int64)
    {
        guard let lineVerticesBuffer = lineVerticesBuffer,
              let lineIndicesBuffer = lineIndicesBuffer
        else { return }

        if stencilState == nil || clearStencilState == nil {
            setupStencilBufferDescriptor()
        }

        // draw call
        encoder.setDepthStencilState(stencilState)
        encoder.setStencilReferenceValue(0xFF)

        shader.setupRectProgram(context)
        shader.preRenderRect(context)

        encoder.setVertexBuffer(lineVerticesBuffer, offset: 0, index: 0)
        let matrixPointer = UnsafeRawPointer(bitPattern: Int(mvpMatrix))!
        encoder.setVertexBytes(matrixPointer, length: 64, index: 1)

        encoder.drawIndexedPrimitives(type: .triangle,
                                      indexCount: indicesCount,
                                      indexType: .uint32,
                                      indexBuffer: lineIndicesBuffer,
                                      indexBufferOffset: 0)
    }
}

extension Line2d: MCLine2dInterface {
    func setLinePositions(_ positions: [MCVec2D]) {
        guard positions.count > 1 else {
            indicesCount = 0
            lineVerticesBuffer = nil
            lineIndicesBuffer = nil
            return
        }

        var lineVertices: [Vertex] = []
        var indices: [UInt32] = []

        for i in 0 ..< (positions.count - 1) {
            let iNext = i + 1

            let ci = positions[i]
            let ciNext = positions[iNext]

            let lineNormalX = -(ciNext.yF - ci.yF)
            let lineNormalY = ciNext.xF - ci.xF
            let lineLength = sqrt(lineNormalX * lineNormalX + lineNormalY * lineNormalY)

            let miter: Float = 100;
            let miterX: Float = lineNormalX / lineLength * miter / 2;
            let miterY: Float = lineNormalY / lineLength * miter / 2;

            let ciX = ci.xF - (ciNext.xF - ci.xF) / lineLength * miter / 2;
            let ciY = ci.yF - (ciNext.yF - ci.yF) / lineLength * miter / 2;

            let ciNextX = ciNext.xF - (ci.xF - ciNext.xF) / lineLength * miter / 2
            let ciNextY = ciNext.yF - (ci.yF - ciNext.yF) / lineLength * miter / 2

            lineVertices.append(contentsOf: [
                Vertex(x: ciX - miterX, y: ciY - miterY, lineStart: ci, lineEnd: ciNext),
                Vertex(x: ciX + miterX, y: ciY + miterY, lineStart: ci, lineEnd: ciNext),
                Vertex(x: ciNextX + miterX, y: ciNextY + miterY, lineStart: ci, lineEnd: ciNext),
                Vertex(x: ciNextX - miterX, y: ciNextY - miterY, lineStart: ci, lineEnd: ciNext)
            ])

            indices.append(contentsOf: [
                UInt32(4 * i), UInt32(4 * i + 1), UInt32(4 * i + 2),
                UInt32(4 * i), UInt32(4 * i + 2), UInt32(4 * i + 3)
            ])
        }

        guard let verticesBuffer = device.makeBuffer(bytes: lineVertices, length: MemoryLayout<Vertex>.stride * lineVertices.count, options: []),
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

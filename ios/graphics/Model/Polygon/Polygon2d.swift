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

class Polygon2d: BaseGraphicsObject {
    private var shader: MCShaderProgramInterface

    private var verticesBuffer: MTLBuffer?
    private var indicesBuffer: MTLBuffer?
    private var indicesCount: Int = 0

    private var stencilState: MTLDepthStencilState?

    init(shader: MCShaderProgramInterface, metalContext: MetalContext) {
        self.shader = shader
        super.init(device: metalContext.device,
                   sampler: metalContext.samplerLibrary.value(Sampler.magLinear.rawValue))
    }

    override func render(encoder: MTLRenderCommandEncoder,
                         context: RenderingContext,
                         renderPass _: MCRenderPassConfig,
                         mvpMatrix: Int64,
                         isMasked: Bool,
                         screenPixelAsRealMeterFactor _: Double)
    {
        guard let verticesBuffer = verticesBuffer,
              let indicesBuffer = indicesBuffer else { return }

        encoder.pushDebugGroup("Polygon2d")

        if isMasked {
            if stencilState == nil {
                setupStencilStates()
            }
            encoder.setDepthStencilState(stencilState)
            if maskInverse {
                encoder.setStencilReferenceValue(0b0000_0000)
            } else {
                encoder.setStencilReferenceValue(0b1000_0000)
            }
        }

        shader.setupProgram(context)
        shader.preRender(context)

        encoder.setVertexBuffer(verticesBuffer, offset: 0, index: 0)
        if let matrixPointer = UnsafeRawPointer(bitPattern: Int(mvpMatrix)) {
            encoder.setVertexBytes(matrixPointer, length: 64, index: 1)
        }

        encoder.drawIndexedPrimitives(type: .triangle,
                                      indexCount: indicesCount,
                                      indexType: .uint16,
                                      indexBuffer: indicesBuffer,
                                      indexBufferOffset: 0)

        encoder.popDebugGroup()
    }

    private func setupStencilStates() {
        let ss2 = MTLStencilDescriptor()
        ss2.stencilCompareFunction = .equal
        ss2.stencilFailureOperation = .zero
        ss2.depthFailureOperation = .keep
        ss2.depthStencilPassOperation = .keep
        ss2.readMask = 0b1111_1111
        ss2.writeMask = 0b0000_0000

        let s2 = MTLDepthStencilDescriptor()
        s2.frontFaceStencil = ss2
        s2.backFaceStencil = ss2

        stencilState = device.makeDepthStencilState(descriptor: s2)
    }
}

extension Polygon2d: MCMaskingObjectInterface {
    func render(asMask context: MCRenderingContextInterface?,
                renderPass _: MCRenderPassConfig,
                mvpMatrix: Int64,
                screenPixelAsRealMeterFactor _: Double) {
        guard let context = context as? RenderingContext,
              let encoder = context.encoder else { return }

        guard let verticesBuffer = verticesBuffer,
              let indicesBuffer = indicesBuffer
        else { return }


        encoder.pushDebugGroup("Polygon2d")

        if stencilState == nil {
            setupStencilStates()
        }

        if let mask = context.polygonMask {
            encoder.setDepthStencilState(mask)
            encoder.setStencilReferenceValue(0b1000_0000)
        }

        // stencil prepare pass
        shader.setupProgram(context)
        shader.preRender(context)

        encoder.setVertexBuffer(verticesBuffer, offset: 0, index: 0)
        if let matrixPointer = UnsafeRawPointer(bitPattern: Int(mvpMatrix)) {
            encoder.setVertexBytes(matrixPointer, length: 64, index: 1)
        }

        encoder.drawIndexedPrimitives(type: .triangle,
                                      indexCount: indicesCount,
                                      indexType: .uint16,
                                      indexBuffer: indicesBuffer,
                                      indexBufferOffset: 0)

        encoder.popDebugGroup()

    }
}

extension Polygon2d: MCPolygon2dInterface {
    func setVertices(_ vertices: [MCVec2D], indices: [NSNumber]) {
        guard !vertices.isEmpty else {
            indicesCount = 0
            verticesBuffer = nil
            indicesBuffer = nil
            return
        }

        let polygonVertices: [Vertex] = vertices.map {
            Vertex(x: $0.xF, y: $0.yF)
        }
        let indices: [UInt16] = indices.map(\.uint16Value)

        guard let verticesBuffer = device.makeBuffer(bytes: polygonVertices, length: MemoryLayout<Vertex>.stride * polygonVertices.count, options: []),
              let indicesBuffer = device.makeBuffer(bytes: indices, length: MemoryLayout<UInt16>.stride * indices.count, options: [])
        else {
            fatalError("Cannot allocate buffers for the UBTileModel")
        }

        indicesCount = indices.count
        self.verticesBuffer = verticesBuffer
        self.indicesBuffer = indicesBuffer
    }

    func asGraphicsObject() -> MCGraphicsObjectInterface? { self }

    func asMaskingObject() -> MCMaskingObjectInterface? { self }
}

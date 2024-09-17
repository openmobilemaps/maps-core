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

final class Polygon2d: BaseGraphicsObject {
    private var shader: MCShaderProgramInterface

    private var verticesBuffer: MTLBuffer?
    private var indicesBuffer: MTLBuffer?
    private var indicesCount: Int = 0

    private var stencilState: MTLDepthStencilState?
    private var renderPassStencilState: MTLDepthStencilState?

    init(shader: MCShaderProgramInterface, metalContext: MetalContext) {
        self.shader = shader
        super.init(device: metalContext.device,
                   sampler: metalContext.samplerLibrary.value(Sampler.magLinear.rawValue)!,
                   label: "Polygon2d")
    }

    override func render(encoder: MTLRenderCommandEncoder,
                         context: RenderingContext,
                         renderPass pass: MCRenderPassConfig,
                         mvpMatrix: Int64,
                         isMasked: Bool,
                         screenPixelAsRealMeterFactor _: Double) {
        lock.lock()
        defer {
            lock.unlock()
        }

        guard let verticesBuffer,
              let indicesBuffer else { return }

#if DEBUG
        encoder.pushDebugGroup(label)
        defer {
            encoder.popDebugGroup()
        }
#endif

        if isMasked {
            if stencilState == nil {
                setupStencilStates()
            }
            encoder.setDepthStencilState(stencilState)
            if maskInverse {
                encoder.setStencilReferenceValue(0b0000_0000)
            } else {
                encoder.setStencilReferenceValue(0b1100_0000)
            }
        }

        if pass.isPassMasked {
            if renderPassStencilState == nil {
                renderPassStencilState = self.renderPassMaskStencilState()
            }

            encoder.setDepthStencilState(renderPassStencilState)
            encoder.setStencilReferenceValue(0b0000_0000)
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
    }

    private func setupStencilStates() {
        let ss2 = MTLStencilDescriptor()
        ss2.stencilCompareFunction = .equal
        ss2.stencilFailureOperation = .zero
        ss2.depthFailureOperation = .keep
        ss2.depthStencilPassOperation = .keep
        ss2.readMask = 0b1100_0000
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
        guard isReady(),
              let context = context as? RenderingContext,
              let encoder = context.encoder else { return }

        lock.lock()
        defer {
            lock.unlock()
        }

        guard let verticesBuffer,
              let indicesBuffer
        else { return }

#if DEBUG
        encoder.pushDebugGroup("Polygon2dMask")
        defer {
            encoder.popDebugGroup()
        }
#endif

        if let mask = context.polygonMask {
            encoder.setStencilReferenceValue(0xFF)
            encoder.setDepthStencilState(mask)
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
    }
}

extension Polygon2d: MCPolygon2dInterface {
    func setVertices(_ vertices: MCSharedBytes, indices: MCSharedBytes) {
        lock.withCritical {
            self.verticesBuffer.copyOrCreate(from: vertices, device: device)
            self.indicesBuffer.copyOrCreate(from: indices, device: device)
            if self.verticesBuffer != nil, self.indicesBuffer != nil {
                self.indicesCount = Int(indices.elementCount)
            } else {
                self.indicesCount = 0
            }
        }
    }

    func asGraphicsObject() -> MCGraphicsObjectInterface? { self }

    func asMaskingObject() -> MCMaskingObjectInterface? { self }
}

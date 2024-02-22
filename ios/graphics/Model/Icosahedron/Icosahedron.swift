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

final class Icosahedron: BaseGraphicsObject {
    private var shader: MCShaderProgramInterface

    private var verticesBuffer: MTLBuffer?

    private var indicesBuffer: MTLBuffer?

    private var indicesCount: Int = 0

    init(shader: MCShaderProgramInterface, metalContext: MetalContext, label: String = "Icosahedron") {
        self.shader = shader
        super.init(device: metalContext.device,
                   sampler: metalContext.samplerLibrary.value(Sampler.magLinear.rawValue)!,
                   label: label)
    }

    override func isReady() -> Bool {
        guard ready else { return false }
        return true
    }

    override func render(encoder: MTLRenderCommandEncoder,
                         context: RenderingContext,
                         renderPass _: MCRenderPassConfig,
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
            if maskInverse {
                encoder.setStencilReferenceValue(0b0000_0000)
            } else {
                encoder.setStencilReferenceValue(0b1100_0000)
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
                                      indexType: .uint32,
                                      indexBuffer: indicesBuffer,
                                      indexBufferOffset: 0)
    }
}

extension Icosahedron: MCMaskingObjectInterface {
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
            encoder.pushDebugGroup("IcosahedronMask")
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
                                      indexType: .uint32,
                                      indexBuffer: indicesBuffer,
                                      indexBufferOffset: 0)
    }
}

extension Icosahedron: MCIcosahedronInterface {
    func setVertices(_ vertices: MCSharedBytes, indices: MCSharedBytes) {
        guard let verticesBuffer = device.makeBuffer(from: vertices),
              let indicesBuffer = device.makeBuffer(from: indices),
              indices.elementCount > 0
        else {
            lock.withCritical {
                indicesCount = 0
                verticesBuffer = nil
                indicesBuffer = nil
            }

            return
        }

        lock.withCritical {
            self.indicesCount = Int(indices.elementCount)
            self.verticesBuffer = verticesBuffer
            self.indicesBuffer = indicesBuffer
        }
    }

    func asGraphicsObject() -> MCGraphicsObjectInterface? { self }

    func asMaskingObject() -> MCMaskingObjectInterface? { self }
}

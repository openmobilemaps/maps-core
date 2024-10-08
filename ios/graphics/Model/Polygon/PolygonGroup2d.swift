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
import simd

final class PolygonGroup2d: BaseGraphicsObject, @unchecked Sendable {
    private var shader: PolygonGroupShader

    private var verticesBuffer: MTLBuffer?
    private var indicesBuffer: MTLBuffer?
    private var indicesCount: Int = 0
    private var origin: MCVec3D?

    private var stencilState: MTLDepthStencilState?
    private var renderPassStencilState: MTLDepthStencilState?
    private var posOffset = SIMD2<Float>([0.0, 0.0])

    init(shader: MCShaderProgramInterface, metalContext: MetalContext) {
        guard let shader = shader as? PolygonGroupShader else {
            fatalError("PolygonGroup2d only supports PolygonGroupShader")
        }
        self.shader = shader
        super.init(device: metalContext.device,
                   sampler: metalContext.samplerLibrary.value(Sampler.magLinear.rawValue)!,
                   label: "PolygonGroup2d")
    }

    override func render(encoder: MTLRenderCommandEncoder,
                         context: RenderingContext,
                         renderPass pass: MCRenderPassConfig,
                         vpMatrix: Int64,
                         mMatrix: Int64,
                         origin: MCVec3D,
                         isMasked: Bool,
                         screenPixelAsRealMeterFactor: Double) {
        lock.lock()
        defer {
            lock.unlock()
        }

        guard let verticesBuffer,
              let indicesBuffer, shader.polygonStyleBuffer != nil,
              let tileOrigin = self.origin
        else { return }


        #if DEBUG
            encoder.pushDebugGroup(label)
            defer {
                encoder.popDebugGroup()
            }
        #endif

        if isMasked {
            if stencilState == nil {
                stencilState = self.maskStencilState()
            }
            encoder.setDepthStencilState(stencilState)
            encoder.setStencilReferenceValue(0b1100_0000)
        }

        if pass.isPassMasked {
            if renderPassStencilState == nil {
                renderPassStencilState = self.renderPassMaskStencilState()
            }

            encoder.setDepthStencilState(renderPassStencilState)
            encoder.setStencilReferenceValue(0b1100_0000)
        }

        shader.setupProgram(context)
        shader.preRender(context)

        encoder.setVertexBuffer(verticesBuffer, offset: 0, index: 0)
        if let matrixPointer = UnsafeRawPointer(bitPattern: Int(vpMatrix)) {
            encoder.setVertexBytes(matrixPointer, length: 64, index: 1)
        }
		
        var originOffset: simd_float4 = simd_float4(
            Float(tileOrigin.x - origin.x),
            Float(tileOrigin.y - origin.y),
            Float(tileOrigin.z - origin.z),
            0
        )
        if let originOffsetBuffer = device.makeBuffer(bytes: &originOffset, length: MemoryLayout<simd_float4>.stride, options: []) {
            encoder.setVertexBuffer(originOffsetBuffer, offset: 0, index: 3)
        }


        if self.shader.isStriped {
            encoder.setVertexBytes(&posOffset, length: MemoryLayout<SIMD2<Float>>.stride, index: 2)

            let p : Float = Float(screenPixelAsRealMeterFactor)
            var scaleFactors = SIMD2<Float>([p, pow(2.0, ceil(log2(p)))])
            encoder.setFragmentBytes(&scaleFactors, length: MemoryLayout<SIMD2<Float>>.stride, index: 2)
        }

        encoder.drawIndexedPrimitives(type: .triangle,
                                      indexCount: indicesCount,
                                      indexType: .uint16,
                                      indexBuffer: indicesBuffer,
                                      indexBufferOffset: 0)
    }
}

extension PolygonGroup2d: MCPolygonGroup2dInterface {
    func setVertices(_ vertices: MCSharedBytes, indices: MCSharedBytes, origin: MCVec3D) {
        guard vertices.elementCount > 0 else {
            lock.withCritical {
                self.indicesCount = 0
                verticesBuffer = nil
                indicesBuffer = nil
                self.origin = nil
            }
            return
        }

        guard let verticesBuffer = device.makeBuffer(from: vertices),
              let indicesBuffer = device.makeBuffer(from: indices)
        else {
            fatalError("Cannot allocate buffers for the UBTileModel")
        }

        var minX = Float.greatestFiniteMagnitude
        var minY = Float.greatestFiniteMagnitude

        if shader.isStriped {
            if let p = UnsafeRawPointer(bitPattern: Int(vertices.address)) {
                for i in 0..<vertices.elementCount {
                    if i % 3 == 0 {
                        let x = (p + 4 * Int(i)).load(as: Float.self)
                        let y = (p + 4 * (Int(i) + 1)).load(as: Float.self)
                        minX = min(x, minX)
                        minY = min(y, minY)
                    }
                }
            }
        }
        lock.withCritical {
            self.indicesCount = Int(indices.elementCount)
            self.verticesBuffer = verticesBuffer
            self.indicesBuffer = indicesBuffer
            self.origin = origin

            if shader.isStriped {
                self.posOffset.x = minX;
                self.posOffset.y = minY;
            }
        }
    }

    func asGraphicsObject() -> MCGraphicsObjectInterface? { self }
}

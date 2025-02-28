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

final class PolygonPatternGroup2d: BaseGraphicsObject, @unchecked Sendable {
    private var shader: PolygonPatternGroupShader

    private var verticesBuffer: MTLBuffer?
    private var indicesBuffer: MTLBuffer?
    private var indicesCount: Int = 0

    private var opacitiesBuffer: MTLBuffer?
    private var textureCoordinatesBuffer: MTLBuffer?

    private var stencilState: MTLDepthStencilState?
    private var renderPassStencilState: MTLDepthStencilState?

    private var texture: MTLTexture?

    private var screenPixelFactor: Float = 0
    var customScreenPixelFactor = SIMD2<Float>([0.0, 0.0])

    var posOffset = SIMD2<Float>([0.0, 0.0])

    init(shader: MCShaderProgramInterface, metalContext: MetalContext) {
        guard let shader = shader as? PolygonPatternGroupShader else {
            fatalError("PolygonPatternGroup2d only supports PolygonPatternGroupShader")
        }
        self.shader = shader
        super.init(device: metalContext.device,
                   sampler: metalContext.samplerLibrary.value(Sampler.magLinear.rawValue)!,
                   label: "PolygonPatternGroup2d")
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
              let indicesBuffer,
              let opacitiesBuffer,
              let textureCoordinatesBuffer,
              let texture else {
            return
        }

#if DEBUG
        encoder.pushDebugGroup(label)
        defer {
            encoder.popDebugGroup()
        }
#endif

        if isMasked {
            if stencilState == nil {
                self.stencilState = self.maskStencilState()
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

        let vpMatrixBuffer = vpMatrixBuffers.getNextBuffer(context)
        if let matrixPointer = UnsafeRawPointer(bitPattern: Int(vpMatrix)) {
            vpMatrixBuffer?.contents().copyMemory(from: matrixPointer, byteCount: 64)
        }
        encoder.setVertexBuffer(vpMatrixBuffer, offset: 0, index: 1)

        // scale factors for shaders
        var pixelFactor: Float = Float(screenPixelAsRealMeterFactor)

        if self.shader.fadeInPattern {
            var scaleFactors = SIMD2<Float>([pixelFactor, pixelFactor])
            encoder.setVertexBytes(&scaleFactors, length: MemoryLayout<SIMD2<Float>>.stride, index: 2)
            encoder.setVertexBytes(&posOffset, length: MemoryLayout<SIMD2<Float>>.stride, index: 3)

            scaleFactors = customScreenPixelFactor.x != 0 ? customScreenPixelFactor : SIMD2<Float>([pixelFactor, pixelFactor])
            encoder.setFragmentBytes(&pixelFactor, length: MemoryLayout<Float>.stride, index: 2)
            encoder.setFragmentBytes(&scaleFactors, length: MemoryLayout<SIMD2<Float>>.stride, index: 3)
        } else {
            var scaleFactors = customScreenPixelFactor.x != 0 ? customScreenPixelFactor : SIMD2<Float>([pixelFactor, pixelFactor])
            encoder.setVertexBytes(&scaleFactors, length: MemoryLayout<SIMD2<Float>>.stride, index: 2)
            encoder.setVertexBytes(&posOffset, length: MemoryLayout<SIMD2<Float>>.stride, index: 3)
        }

        // texture
        encoder.setFragmentTexture(texture, index: 0)
        encoder.setFragmentSamplerState(sampler, index: 0)

        encoder.setFragmentBuffer(opacitiesBuffer, offset: 0, index: 0)
        encoder.setFragmentBuffer(textureCoordinatesBuffer, offset: 0, index: 1)

        let originOffsetBuffer = originOffsetBuffers.getNextBuffer(context)
        if let bufferPointer = originOffsetBuffer?.contents().assumingMemoryBound(to: simd_float4.self) {
            bufferPointer.pointee.x = Float(originOffset.x - origin.x)
            bufferPointer.pointee.y = Float(originOffset.y - origin.y)
            bufferPointer.pointee.z = Float(originOffset.z - origin.z)
        }
        else {
            fatalError()
        }
        encoder.setVertexBuffer(originOffsetBuffer, offset: 0, index: 4)

        encoder.drawIndexedPrimitives(type: .triangle,
                                      indexCount: indicesCount,
                                      indexType: .uint16,
                                      indexBuffer: indicesBuffer,
                                      indexBufferOffset: 0)
    }
}

extension PolygonPatternGroup2d: MCPolygonPatternGroup2dInterface {
    func setVertices(_ vertices: MCSharedBytes, indices: MCSharedBytes, origin: MCVec3D) {
        guard vertices.elementCount > 0 else {
            lock.withCritical {
                self.indicesCount = 0
                verticesBuffer = nil
                indicesBuffer = nil
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

        if let rawPointer = UnsafeRawPointer(bitPattern: Int(vertices.address)) {
            let count = Int(vertices.elementCount / 4)
            let float4Pointer = rawPointer.assumingMemoryBound(to: SIMD4<Float>.self)
            for i in 0 ..< count {
                let value = float4Pointer[i]
                minX = min(value.x, minX)
                minY = min(value.y, minY)
            }
        }
        lock.withCritical {
            self.indicesCount = Int(indices.elementCount)
            self.verticesBuffer = verticesBuffer
            self.indicesBuffer = indicesBuffer
            self.posOffset.x = minX
            self.posOffset.y = minY
            self.originOffset = origin
        }
    }

    func setOpacities(_ values: MCSharedBytes) {
        lock.withCritical {
            opacitiesBuffer.copyOrCreate(from: values, device: device)
        }
    }

    func setTextureCoordinates(_ values: MCSharedBytes) {
        lock.withCritical {
            textureCoordinatesBuffer.copyOrCreate(from: values, device: device)
        }
    }

    func setScalingFactors(_ factor: MCVec2F) {
        lock.withCritical {
            customScreenPixelFactor.x = factor.x
            customScreenPixelFactor.y = factor.y
        }
    }

    func setScalingFactor(_ factor: Float) {
        lock.withCritical {
            customScreenPixelFactor.x = factor
            customScreenPixelFactor.y = factor
        }
    }

    func loadTexture(_ context: MCRenderingContextInterface?, textureHolder: MCTextureHolderInterface?) {
        guard let textureHolder = textureHolder as? TextureHolder else {
            fatalError("unexpected TextureHolder")
        }
        lock.withCritical {
            texture = textureHolder.texture
            ready = true
        }
    }

    func removeTexture() {
        lock.withCritical {
            texture = nil
        }
    }

    func asGraphicsObject() -> MCGraphicsObjectInterface? { self }
}

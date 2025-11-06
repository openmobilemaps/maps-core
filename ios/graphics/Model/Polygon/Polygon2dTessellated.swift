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
@preconcurrency import Metal
import simd

final class Polygon2dTessellated: BaseGraphicsObject, @unchecked Sendable {
    private var shader: MCShaderProgramInterface

    private var verticesBuffer: MTLBuffer?
    private var indicesBuffer: MTLBuffer?
    private var indicesCount: Int = 0
    
    private var tessellationFactorsBuffer: MTLBuffer?

    private var stencilState: MTLDepthStencilState?
    private var renderPassStencilState: MTLDepthStencilState?

    init(shader: MCShaderProgramInterface, metalContext: MetalContext) {
        self.shader = shader
        super
            .init(
                device: metalContext.device,
                sampler: metalContext.samplerLibrary.value(
                    Sampler.magLinear.rawValue)!,
                label: "Polygon2dTessellated")

    }

    override func render(
        encoder: MTLRenderCommandEncoder,
        context: RenderingContext,
        renderPass pass: MCRenderPassConfig,
        vpMatrix: Int64,
        mMatrix: Int64,
        origin: MCVec3D,
        isMasked: Bool,
        screenPixelAsRealMeterFactor _: Double,
        isScreenSpaceCoords: Bool
    ) {
        lock.lock()
        defer {
            lock.unlock()
        }

        guard let verticesBuffer,
            let indicesBuffer,
            let tessellationFactorsBuffer
        else { return }

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
        shader.preRender(context, isScreenSpaceCoords: isScreenSpaceCoords)

        encoder.setVertexBuffer(verticesBuffer, offset: 0, index: 0)

        let vpMatrixBuffer = vpMatrixBuffers.getNextBuffer(context)
        if let matrixPointer = UnsafeRawPointer(bitPattern: Int(vpMatrix)) {
            vpMatrixBuffer?.contents()
                .copyMemory(
                    from: matrixPointer, byteCount: 64)
        }
        encoder.setVertexBuffer(vpMatrixBuffer, offset: 0, index: 1)

        if shader.usesModelMatrix() {
            if let mMatrixPointer = UnsafeRawPointer(bitPattern: Int(mMatrix)) {
                encoder.setVertexBytes(mMatrixPointer, length: 64, index: 2)
            }
        }

        let originOffsetBuffer = originOffsetBuffers.getNextBuffer(context)
        if let bufferPointer = originOffsetBuffer?.contents()
            .assumingMemoryBound(to: simd_float4.self)
        {
            bufferPointer.pointee.x = Float(originOffset.x - origin.x)
            bufferPointer.pointee.y = Float(originOffset.y - origin.y)
            bufferPointer.pointee.z = Float(originOffset.z - origin.z)
        }
        encoder.setVertexBuffer(originOffsetBuffer, offset: 0, index: 3)
         
        encoder.setTessellationFactorBuffer(tessellationFactorsBuffer, offset: 0, instanceStride: 0)
        
        encoder.drawIndexedPatches(
            numberOfPatchControlPoints: 3,
            patchStart: 0,
            patchCount: indicesCount / 3,
            patchIndexBuffer: nil,
            patchIndexBufferOffset: 0,
            controlPointIndexBuffer: indicesBuffer,
            controlPointIndexBufferOffset: 0,
            instanceCount: 1,
            baseInstance: 0)
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

extension Polygon2dTessellated: MCMaskingObjectInterface {
    func render(
        asMask context: MCRenderingContextInterface?,
        renderPass _: MCRenderPassConfig,
        vpMatrix: Int64,
        mMatrix: Int64,
        origin: MCVec3D,
        screenPixelAsRealMeterFactor _: Double,
        isScreenSpaceCoords: Bool
    ) {

        lock.lock()
        defer {
            lock.unlock()
        }

        guard isReady(),
            let context = context as? RenderingContext,
            let encoder = context.encoder
        else { return }

        guard let verticesBuffer,
            let indicesBuffer,
            let tessellationFactorsBuffer
        else { return }

        #if DEBUG
            encoder.pushDebugGroup("Polygon2dTessellated")
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
        shader.preRender(context, isScreenSpaceCoords: isScreenSpaceCoords)

        encoder.setVertexBuffer(verticesBuffer, offset: 0, index: 0)

        let vpMatrixBuffer = vpMatrixBuffers.getNextBuffer(context)
        if let matrixPointer = UnsafeRawPointer(bitPattern: Int(vpMatrix)) {
            vpMatrixBuffer?.contents()
                .copyMemory(
                    from: matrixPointer, byteCount: 64)
        }
        encoder.setVertexBuffer(vpMatrixBuffer, offset: 0, index: 1)

        if shader.usesModelMatrix() {
            if let mMatrixPointer = UnsafeRawPointer(bitPattern: Int(mMatrix)) {
                encoder.setVertexBytes(mMatrixPointer, length: 64, index: 2)
            }
        }

        let originOffsetBuffer = originOffsetBuffers.getNextBuffer(context)
        if let bufferPointer = originOffsetBuffer?.contents()
            .assumingMemoryBound(to: simd_float4.self)
        {
            bufferPointer.pointee.x = Float(originOffset.x - origin.x)
            bufferPointer.pointee.y = Float(originOffset.y - origin.y)
            bufferPointer.pointee.z = Float(originOffset.z - origin.z)
        }
        encoder.setVertexBuffer(originOffsetBuffer, offset: 0, index: 3)
         
        encoder.setTessellationFactorBuffer(tessellationFactorsBuffer, offset: 0, instanceStride: 0)
        
        encoder.drawIndexedPatches(
            numberOfPatchControlPoints: 3,
            patchStart: 0,
            patchCount: indicesCount / 3,
            patchIndexBuffer: nil,
            patchIndexBufferOffset: 0,
            controlPointIndexBuffer: indicesBuffer,
            controlPointIndexBufferOffset: 0,
            instanceCount: 1,
            baseInstance: 0)
    }
}

extension Polygon2dTessellated: MCPolygon2dInterface {
    func setVertices(
        _ vertices: MCSharedBytes, indices: MCSharedBytes, origin: MCVec3D
    ) {
        lock.withCritical {
            self.verticesBuffer.copyOrCreate(from: vertices, device: device)
            self.indicesBuffer.copyOrCreate(from: indices, device: device)
            if self.verticesBuffer != nil, self.indicesBuffer != nil {
                self.indicesCount = Int(indices.elementCount)
            } else {
                self.indicesCount = 0
            }
            self.originOffset = origin
            
            // todo determine from cpu version
            let tessellationFactors: [Float16] = [
                2, // edge 0
                2, // edge 1
                2, // edge 2
                2, // inside 0
            ]
            self.tessellationFactorsBuffer.copyOrCreate(
                bytes: tessellationFactors,
                length: MemoryLayout<Float16>.stride * tessellationFactors.count,
                device: device)
        }
    }

    func asGraphicsObject() -> MCGraphicsObjectInterface? { self }

    func asMaskingObject() -> MCMaskingObjectInterface? { self }
}

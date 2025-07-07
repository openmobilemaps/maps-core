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

final class TextInstanced: BaseGraphicsObject, @unchecked Sendable {
    private var shader: TextInstancedShader

    private var verticesBuffer: MTLBuffer?
    private var indicesBuffer: MTLBuffer?
    private var indicesCount: Int = 0

    private var instanceCount: Int = 0
    private var positionsBuffer: MTLBuffer?
    private var alphasBuffer: MTLBuffer?
    private var referencePositionsBuffer: MTLBuffer?
    private var textureCoordinatesBuffer: MTLBuffer?
    private var scalesBuffer: MTLBuffer?
    private var rotationsBuffer: MTLBuffer?
    private var styleIndicesBuffer: MTLBuffer?
    private var styleBuffer: MTLBuffer?
    private var originBuffers: MultiBuffer<simd_float4>
    private var aspectRatioBuffers: MultiBuffer<simd_float1>

    private var texture: MTLTexture?

    private var stencilState: MTLDepthStencilState?

    init(shader: MCShaderProgramInterface, metalContext: MetalContext) {
        self.shader = shader as! TextInstancedShader
        self.originBuffers = .init(device: metalContext.device)
        self.aspectRatioBuffers = .init(device: metalContext.device)
        super
            .init(
                device: metalContext.device,
                sampler: metalContext.samplerLibrary.value(
                    Sampler.magLinear.rawValue)!,
                label: "TextInstanced")
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

    override func render(
        encoder: MTLRenderCommandEncoder,
        context: RenderingContext,
        renderPass _: MCRenderPassConfig,
        vpMatrix: Int64,
        mMatrix: Int64,
        origin: MCVec3D,
        isMasked: Bool,
        screenPixelAsRealMeterFactor _: Double
    ) {
        lock.lock()
        defer {
            lock.unlock()
        }

        guard let verticesBuffer,
            let indicesBuffer,
            let positionsBuffer,
            let scalesBuffer,
            let rotationsBuffer,
            let textureCoordinatesBuffer,
            let styleIndicesBuffer,
            let styleBuffer,
            instanceCount != 0
        else { return }

        if isMasked {
            if stencilState == nil {
                setupStencilStates()
            }
            encoder.setDepthStencilState(stencilState)
            encoder.setStencilReferenceValue(0b1000_0000)
        } else {
            encoder.setDepthStencilState(context.defaultMask)
        }

        #if DEBUG
            encoder.pushDebugGroup(label + "-Halo")
        #endif

        shader.setupProgram(context)
        shader.preRender(context)

        encoder.setVertexBuffer(verticesBuffer, offset: 0, index: 0)

        let vpMatrixBuffer = vpMatrixBuffers.getNextBuffer(context)
        if let matrixPointer = UnsafeRawPointer(bitPattern: Int(vpMatrix)) {
            vpMatrixBuffer?.contents()
                .copyMemory(
                    from: matrixPointer, byteCount: 64)
        }
        encoder.setVertexBuffer(vpMatrixBuffer, offset: 0, index: 1)

        if let mMatrixPointer = UnsafeRawPointer(bitPattern: Int(mMatrix)) {
            encoder.setVertexBytes(mMatrixPointer, length: 64, index: 2)
        }

        encoder.setVertexBuffer(positionsBuffer, offset: 0, index: 3)
        encoder.setVertexBuffer(scalesBuffer, offset: 0, index: 4)
        encoder.setVertexBuffer(rotationsBuffer, offset: 0, index: 5)
        encoder.setVertexBuffer(textureCoordinatesBuffer, offset: 0, index: 6)
        encoder.setVertexBuffer(styleIndicesBuffer, offset: 0, index: 7)
        encoder.setVertexBuffer(alphasBuffer, offset: 0, index: 12)

        if shader.isUnitSphere,
            let referencePositionsBuffer
        {
            encoder.setVertexBuffer(
                referencePositionsBuffer, offset: 0, index: 8)
        }

        let originOffsetBuffer = originOffsetBuffers.getNextBuffer(context)
        if let bufferPointer = originOffsetBuffer?.contents()
            .assumingMemoryBound(to: simd_float4.self)
        {
            bufferPointer.pointee.x = Float(originOffset.x - origin.x)
            bufferPointer.pointee.y = Float(originOffset.y - origin.y)
            bufferPointer.pointee.z = Float(originOffset.z - origin.z)
        } else {
            fatalError()
        }
        encoder.setVertexBuffer(originOffsetBuffer, offset: 0, index: 9)

        let originBuffer = originBuffers.getNextBuffer(context)
        if let bufferPointer = originBuffer?.contents()
            .assumingMemoryBound(
                to: simd_float4.self)
        {
            bufferPointer.pointee.x = Float(origin.x)
            bufferPointer.pointee.y = Float(origin.y)
            bufferPointer.pointee.z = Float(origin.z)
        } else {
            fatalError()
        }
        encoder.setVertexBuffer(originBuffer, offset: 0, index: 10)

        let aspectRatioBuffer = aspectRatioBuffers.getNextBuffer(context)
        if let bufferPointer = aspectRatioBuffer?.contents()
            .assumingMemoryBound(to: simd_float1.self)
        {
            bufferPointer.pointee = Float(context.aspectRatio)
        } else {
            fatalError()
        }
        encoder.setVertexBuffer(aspectRatioBuffer, offset: 0, index: 11)

        encoder.setFragmentSamplerState(sampler, index: 0)

        if let texture {
            encoder.setFragmentTexture(texture, index: 0)
        }

        encoder.setFragmentBuffer(styleBuffer, offset: 0, index: 1)

        var isHalo: Bool = true

        encoder.setFragmentBytes(
            &isHalo, length: MemoryLayout<Bool>.stride, index: 2)

        encoder.drawIndexedPrimitives(
            type: .triangle,
            indexCount: indicesCount,
            indexType: .uint16,
            indexBuffer: indicesBuffer,
            indexBufferOffset: 0,
            instanceCount: instanceCount)
        isHalo = false
        #if DEBUG
            encoder.popDebugGroup()
            encoder.pushDebugGroup(label)
            defer {
                encoder.popDebugGroup()
            }
        #endif

        encoder.setFragmentBytes(
            &isHalo, length: MemoryLayout<Bool>.stride, index: 2)

        encoder.drawIndexedPrimitives(
            type: .triangle,
            indexCount: indicesCount,
            indexType: .uint16,
            indexBuffer: indicesBuffer,
            indexBufferOffset: 0,
            instanceCount: instanceCount)
    }
}

extension TextInstanced: MCTextInstancedInterface {
    func setFrame(_ frame: MCQuad2dD, origin: MCVec3D, is3d: Bool) {
        /*
         The quad is made out of 4 vertices as following
         B----C
         |    |
         |    |
         A----D
         Where A-C are joined to form two triangles
         */
        let vertecies: [Vertex] = [
            Vertex(position: frame.bottomLeft, textureU: 0, textureV: 1),  // A
            Vertex(position: frame.topLeft, textureU: 0, textureV: 0),  // B
            Vertex(position: frame.topRight, textureU: 1, textureV: 0),  // C
            Vertex(position: frame.bottomRight, textureU: 1, textureV: 1),  // D
        ]
        let indices: [UInt16] = [
            0, 2, 1,  // ACB
            0, 3, 2,  // ADC
        ]

        guard
            let verticesBuffer = device.makeBuffer(
                bytes: vertecies,
                length: MemoryLayout<Vertex>.stride * vertecies.count,
                options: []),
            let indicesBuffer = device.makeBuffer(
                bytes: indices,
                length: MemoryLayout<UInt16>.stride * indices.count, options: []
            )
        else {
            fatalError("Cannot allocate buffers")
        }

        lock.withCritical {
            self.originOffset = origin
            self.indicesCount = indices.count
            self.verticesBuffer = verticesBuffer
            self.indicesBuffer = indicesBuffer
        }
    }

    func loadFont(
        _ context: MCRenderingContextInterface?,
        fontData: MCFontData,
        fontMsdfTexture: MCTextureHolderInterface?
    ) {
        guard let textureHolder = fontMsdfTexture as? TextureHolder else {
            fatalError("unexpected TextureHolder")
        }

        lock.withCritical {
            texture = textureHolder.texture
        }
    }

    func setInstanceCount(_ count: Int32) {
        lock.withCritical {
            self.instanceCount = Int(count)
        }
    }

    func setAlphas(_ alphas: MCSharedBytes) {
        lock.withCritical {
            alphasBuffer.copyOrCreate(
                from: alphas, device: device)
        }
    }

    func setReferencePositions(_ positions: MCSharedBytes) {
        lock.withCritical {
            referencePositionsBuffer.copyOrCreate(
                from: positions, device: device)
        }
    }

    func setPositions(_ positions: MCSharedBytes) {
        lock.withCritical {
            positionsBuffer.copyOrCreate(from: positions, device: device)
        }
    }

    func setTextureCoordinates(_ textureCoordinates: MCSharedBytes) {
        lock.withCritical {
            textureCoordinatesBuffer.copyOrCreate(
                from: textureCoordinates, device: device)
        }
    }

    func setScales(_ scales: MCSharedBytes) {
        lock.withCritical {
            scalesBuffer.copyOrCreate(from: scales, device: device)
        }
    }

    func setRotations(_ rotations: MCSharedBytes) {
        lock.withCritical {
            rotationsBuffer.copyOrCreate(from: rotations, device: device)
        }
    }

    func setStyleIndices(_ indices: MCSharedBytes) {
        lock.withCritical {
            styleIndicesBuffer.copyOrCreate(from: indices, device: device)
        }
    }

    func setStyles(_ values: MCSharedBytes) {
        lock.withCritical {
            styleBuffer.copyOrCreate(from: values, device: device)
        }
    }

    func removeTexture() {
        lock.withCritical {
            texture = nil
        }
    }

    func asGraphicsObject() -> MCGraphicsObjectInterface? {
        self
    }
}

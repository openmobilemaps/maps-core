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

final class Polygon3d: BaseGraphicsObject {
    private var shader: MCShaderProgramInterface

    private var verticesBuffer: MTLBuffer?
    private var indicesBuffer: MTLBuffer?
    private var indicesCount: Int = 0

    private var stencilState: MTLDepthStencilState?

    private var texture: MTLTexture?
    private var heightTexture: MTLTexture?
    public let heightSampler: MTLSamplerState
    
    private var tessellationFactorBuffer: MTLBuffer?
    
    private static let renderStartTime = Date()

    fileprivate var layerOffset: Float = 0

    #if DEBUG
        private var label: String
    #endif

    init(shader: MCShaderProgramInterface, metalContext: MetalContext, label: String = "Polygon3d") {
#if DEBUG
        self.label = label
#endif
        self.shader = shader

        self.heightSampler = metalContext.samplerLibrary.value(Sampler.magNearest.rawValue)


        super.init(device: metalContext.device,
                   sampler: metalContext.samplerLibrary.value(Sampler.magLinear.rawValue))
    }

    override func render(encoder: MTLRenderCommandEncoder,
                         context: RenderingContext,
                         renderPass: MCRenderPassConfig,
                         mvpMatrix: Int64,
                         isMasked: Bool,
                         screenPixelAsRealMeterFactor _: Double) {

        guard isReady(),
              let verticesBuffer = verticesBuffer,
              let indicesBuffer = indicesBuffer,
              let tessellationFactorBuffer = tessellationFactorBuffer
        else {
            return
        }

        lock.lock()
        defer {
            lock.unlock()
        }

        if shader is AlphaShader, texture == nil {
            ready = false
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
                setupStencilStates()
            }
            encoder.setDepthStencilState(stencilState)
            if maskInverse {
                encoder.setStencilReferenceValue(0b0000_0000)
            } else {
                encoder.setStencilReferenceValue(0b1100_0000)
            }
        }else {
            encoder.setDepthStencilState(context.defaultMask3d)
        }

        shader.setupProgram(context)
        shader.preRender(context, pass: renderPass)
        encoder.setCullMode(.back)

        encoder.setVertexBuffer(verticesBuffer, offset: 0, index: 0)
        if let matrixPointer = UnsafeRawPointer(bitPattern: Int(mvpMatrix)) {
            encoder.setVertexBytes(matrixPointer, length: 64, index: 1)
        }

        encoder.setFragmentSamplerState(sampler, index: 0)
        encoder.setVertexSamplerState(heightSampler, index: 0)

        encoder.setFragmentTexture(texture, index: 0)
        encoder.setVertexTexture(heightTexture, index: 0)

        var time = Float(-Self.renderStartTime.timeIntervalSinceNow)
        encoder.setVertexBytes(&time, length: MemoryLayout<Float>.stride, index: 2)
        encoder.setVertexBytes(&layerOffset, length: MemoryLayout<Int32>.stride, index: 3)


        encoder.setTessellationFactorBuffer(tessellationFactorBuffer, offset: 0, instanceStride: 0)

        encoder.drawIndexedPatches(numberOfPatchControlPoints: 3, patchStart: 0, patchCount: self.indicesCount / 3, patchIndexBuffer: nil, patchIndexBufferOffset: 0, controlPointIndexBuffer: indicesBuffer, controlPointIndexBufferOffset: 0, instanceCount: 1, baseInstance: 0)

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
        s2.depthCompareFunction = .lessEqual
        s2.isDepthWriteEnabled = true

        stencilState = device.makeDepthStencilState(descriptor: s2)
    }

    func createTessellationFactors(patchCount: Int) {
        let device = MetalContext.current.device
        let library = MetalContext.current.library

        guard let tessellationBuffer = device.makeBuffer(length: MemoryLayout<MTLQuadTessellationFactorsHalf>.stride * patchCount, options: .storageModePrivate) else {
            fatalError("Cannot allocate buffer for Tessellation")
        }

        lock.withCritical {
            self.tessellationFactorBuffer = tessellationBuffer
        }

        guard let computeFunction = library.makeFunction(name: "compute_tess_factors"),
              let computePipelineState = try? device.makeComputePipelineState(function: computeFunction) else {
            fatalError("Cannot locate the shaders for UBTileModel")
        }

        let commandBuffer = MetalContext.current.commandQueue.makeCommandBuffer()
        guard let computeCommandEncoder = commandBuffer?.makeComputeCommandEncoder() else {
            fatalError("Cannot locate the shaders for UBTileModel")
        }

        computeCommandEncoder.setComputePipelineState(computePipelineState)
        computeCommandEncoder.setBuffer(tessellationFactorBuffer, offset: 0, index: 0)

        let threadgroupSize = MTLSize(width: computePipelineState.threadExecutionWidth, height: 1, depth: 1)
        let threadCount = MTLSize(width: patchCount, height: 1, depth: 1)
        computeCommandEncoder.dispatchThreads(threadCount, threadsPerThreadgroup: threadgroupSize)

        computeCommandEncoder.endEncoding()

        commandBuffer?.commit()
    }
}

extension Polygon3d: MCMaskingObjectInterface {
    func setTileInfo(_ x: Int32, y: Int32, z: Int32, offset: Int32) {
        self.layerOffset = Float(offset)
#if DEBUG
        lock.withCritical {
            label = "Polygon3d (\(z)/\(x)/\(y), \(offset))"
        }
#endif
    }
    func render(asMask context: MCRenderingContextInterface?,
                renderPass: MCRenderPassConfig,
                mvpMatrix: Int64,
                screenPixelAsRealMeterFactor _: Double) {
        guard isReady(),
              let context = context as? RenderingContext,
              let encoder = context.encoder else { return }

        lock.lock()
        defer {
            lock.unlock()
        }

        guard let verticesBuffer = verticesBuffer,
              let indicesBuffer = indicesBuffer,
              let tessellationFactorBuffer = tessellationFactorBuffer
        else { return }

#if DEBUG
        encoder.pushDebugGroup("Polygon3dMask")
        defer {
            encoder.popDebugGroup()
        }
#endif

        if let mask = context.polygonMask3d {
            encoder.setStencilReferenceValue(0xFF)
            encoder.setDepthStencilState(mask)
        }

        // stencil prepare pass
        shader.setupProgram(context)
        shader.preRender(context, pass: renderPass)

        encoder.setVertexBuffer(verticesBuffer, offset: 0, index: 0)
        if let matrixPointer = UnsafeRawPointer(bitPattern: Int(mvpMatrix)) {
            encoder.setVertexBytes(matrixPointer, length: 64, index: 1)
        }

        var time = Float(-Self.renderStartTime.timeIntervalSinceNow)
        encoder.setVertexBytes(&time, length: MemoryLayout<Float>.stride, index: 2)


        encoder.setTessellationFactorBuffer(tessellationFactorBuffer, offset: 0, instanceStride: 0)

        encoder.setFragmentSamplerState(sampler, index: 0)
        encoder.setVertexSamplerState(sampler, index: 0)

        encoder.drawIndexedPatches(numberOfPatchControlPoints: 3, patchStart: 0, patchCount: self.indicesCount / 3, patchIndexBuffer: nil, patchIndexBufferOffset: 0, controlPointIndexBuffer: indicesBuffer, controlPointIndexBufferOffset: 0, instanceCount: 1, baseInstance: 0)

    }
}

extension Polygon3d: MCPolygon3dInterface {
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

        createTessellationFactors(patchCount: Int(indices.elementCount))
    }

    func asGraphicsObject() -> MCGraphicsObjectInterface? { self }

    func asMaskingObject() -> MCMaskingObjectInterface? { self }

    func loadTexture(_ context: MCRenderingContextInterface?, textureHolder: MCTextureHolderInterface?) {
        guard let textureHolder = textureHolder as? TextureHolder else {
            fatalError("unexpected TextureHolder")
        }
        texture = textureHolder.texture
    }

    func loadHeightTexture(_ context: MCRenderingContextInterface?, textureHolder: MCTextureHolderInterface?) {
        guard let textureHolder = textureHolder as? TextureHolder else {
            fatalError("unexpected TextureHolder")
        }
        heightTexture = textureHolder.texture
    }

    func removeTexture() {}
}

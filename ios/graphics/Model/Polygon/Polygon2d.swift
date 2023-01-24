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

    private let timeBuffer: MTLBuffer
    private var timeBufferContent : UnsafeMutablePointer<Float>

    private var tessellationFactorBuffer: MTLBuffer?

    private var stencilState: MTLDepthStencilState?

    init(shader: MCShaderProgramInterface, metalContext: MetalContext) {
        self.shader = shader
        guard let buffer = MetalContext.current.device.makeBuffer(length: MemoryLayout<Float>.stride, options: []) else { fatalError("Could not create buffer") }
        self.timeBuffer = buffer
        self.timeBufferContent = self.timeBuffer.contents().bindMemory(to: Float.self, capacity: 1)

        super.init(device: metalContext.device,
                   sampler: metalContext.samplerLibrary.value(Sampler.magLinear.rawValue))
    }

    // TODO: Move
    static let startTime = Date()

    override func render(encoder: MTLRenderCommandEncoder,
                         context: RenderingContext,
                         renderPass: MCRenderPassConfig,
                         mvpMatrix: Int64,
                         isMasked: Bool,
                         screenPixelAsRealMeterFactor _: Double) {
        lock.lock()
        defer {
            lock.unlock()
        }
        
        guard let verticesBuffer = verticesBuffer,
              let indicesBuffer = indicesBuffer else { return }

        #if DEBUG
        encoder.pushDebugGroup("Polygon2d")
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

        shader.setupProgram(context)
        shader.preRender(context, pass: renderPass)

        encoder.setVertexBuffer(verticesBuffer, offset: 0, index: 0)
        if let matrixPointer = UnsafeRawPointer(bitPattern: Int(mvpMatrix)) {
            encoder.setVertexBytes(matrixPointer, length: 64, index: 1)
        }

        timeBufferContent[0] = Float(-Self.startTime.timeIntervalSinceNow)
        encoder.setVertexBuffer(timeBuffer, offset: 0, index: 2)

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
                renderPass: MCRenderPassConfig,
                mvpMatrix: Int64,
                screenPixelAsRealMeterFactor _: Double) {
        guard isReady(),
              let context = context as? RenderingContext,
              let encoder = context.encoder(pass: renderPass) else { return }

        lock.lock()
        defer {
            lock.unlock()
        }

        guard let verticesBuffer = verticesBuffer,
              let indicesBuffer = indicesBuffer,
              let tessellationFactorBuffer = tessellationFactorBuffer
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
        shader.preRender(context, pass: renderPass)

        encoder.setVertexBuffer(verticesBuffer, offset: 0, index: 0)
        if let matrixPointer = UnsafeRawPointer(bitPattern: Int(mvpMatrix)) {
            encoder.setVertexBytes(matrixPointer, length: 64, index: 1)
        }

        timeBufferContent[0] = Float(-Self.startTime.timeIntervalSinceNow)
        encoder.setVertexBuffer(timeBuffer, offset: 0, index: 2)




        if shader.usesTessellation() {
            encoder.setTessellationFactorBuffer(tessellationFactorBuffer, offset: 0, instanceStride: 0)

            encoder.drawIndexedPatches(numberOfPatchControlPoints: 3, patchStart: 0, patchCount: indicesCount/3, patchIndexBuffer: nil, patchIndexBufferOffset: 0, controlPointIndexBuffer: indicesBuffer, controlPointIndexBufferOffset: 0, instanceCount: 1, baseInstance: 0)

        }
        else {
            encoder.drawIndexedPrimitives(type: .triangle,
                                          indexCount: indicesCount,
                                          indexType: .uint16,
                                          indexBuffer: indicesBuffer,
                                          indexBufferOffset: 0)

        }

    }
}

extension Polygon2d: MCPolygon2dInterface {
    func setVertices(_ vertices: [MCVec2D], indices: [NSNumber]) {
        guard !vertices.isEmpty, !indices.isEmpty else {
            lock.withCritical {
                indicesCount = 0
                verticesBuffer = nil
                indicesBuffer = nil
            }
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

        lock.withCritical {
            indicesCount = indices.count
            self.verticesBuffer = verticesBuffer
            self.indicesBuffer = indicesBuffer
        }

        if shader.usesTessellation() {
            createTessellationFactors(patchCount: indices.count)
        }
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

    func asGraphicsObject() -> MCGraphicsObjectInterface? { self }

    func asMaskingObject() -> MCMaskingObjectInterface? { self }
}

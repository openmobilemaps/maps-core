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

final class Quad3d: BaseGraphicsObject {
    private var verticesBuffer: MTLBuffer?

    private var indicesBuffer: MTLBuffer?

    private var indicesCount: Int = 0

    private var texture: MTLTexture?

    private var shader: MCShaderProgramInterface

    private var stencilState: MTLDepthStencilState?

    private var renderAsMask = false

    private let label: String

    private let timeBuffer: MTLBuffer
    private var timeBufferContent : UnsafeMutablePointer<Float>

    private var tessellationFactorBuffer: MTLBuffer?

    private static let renderStartTime = Date()

    init(shader: MCShaderProgramInterface, metalContext: MetalContext, label: String = "Quad3d") {
        self.label = label
        self.shader = shader

        guard let timeBuffer = MetalContext.current.device.makeBuffer(length: MemoryLayout<Float>.stride, options: []) else { fatalError("Could not create buffer") }
        self.timeBuffer = timeBuffer
        self.timeBufferContent = self.timeBuffer.contents().bindMemory(to: Float.self, capacity: 1)

        super.init(device: metalContext.device,
                   sampler: metalContext.samplerLibrary.value(Sampler.magLinear.rawValue))
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

    override func isReady() -> Bool {
        guard ready else { return false }
        if shader is AlphaShader {
            return texture != nil
        }
        return true
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
        else { return }

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
            encoder.setStencilReferenceValue(0b1100_0000)
        } else if let mask = context.mask, renderAsMask {
            encoder.setDepthStencilState(mask)
            encoder.setStencilReferenceValue(0b1100_0000)
        } else {
            encoder.setDepthStencilState(context.defaultMask)
        }

        shader.setupProgram(context)
        shader.preRender(context, pass: renderPass)

        encoder.setCullMode(.back)

        encoder.setVertexBuffer(verticesBuffer, offset: 0, index: 0)
        if let matrixPointer = UnsafeRawPointer(bitPattern: Int(mvpMatrix)) {
            encoder.setVertexBytes(matrixPointer, length: 64, index: 1)
        }

        encoder.setFragmentSamplerState(sampler, index: 0)

        if let texture = texture {
            encoder.setFragmentTexture(texture, index: 0)
        }

        timeBufferContent[0] = Float(-Self.renderStartTime.timeIntervalSinceNow)
        encoder.setVertexBuffer(timeBuffer, offset: 0, index: 2)


        encoder.setTessellationFactorBuffer(tessellationFactorBuffer, offset: 0, instanceStride: 0)

        encoder.drawIndexedPatches(numberOfPatchControlPoints: 3, patchStart: 0, patchCount: 2, patchIndexBuffer: nil, patchIndexBufferOffset: 0, controlPointIndexBuffer: indicesBuffer, controlPointIndexBufferOffset: 0, instanceCount: 1, baseInstance: 0)

    }
}

extension Quad3d: MCMaskingObjectInterface {
    func render(asMask context: MCRenderingContextInterface?,
                renderPass: MCRenderPassConfig,
                mvpMatrix: Int64,
                screenPixelAsRealMeterFactor: Double) {
        guard isReady(),
              let context = context as? RenderingContext,
              let encoder = context.encoder else { return }

        renderAsMask = true

        render(encoder: encoder,
               context: context,
               renderPass: renderPass,
               mvpMatrix: mvpMatrix,
               isMasked: false,
               screenPixelAsRealMeterFactor: screenPixelAsRealMeterFactor)
    }

}

extension Quad3d: MCQuad3dInterface {
    func setFrame(_ frame: MCQuad2dD, textureCoordinates: MCRectD) {
        /*
         The quad is made out of 4 vertices as following
         B----C
         |    |
         |    |
         A----D
         Where A-C are joined to form two triangles
         */
        let vertecies: [Vertex] = [
            Vertex(position: frame.bottomLeft, textureU: textureCoordinates.xF, textureV: textureCoordinates.yF + textureCoordinates.heightF), // A
            Vertex(position: frame.topLeft, textureU: textureCoordinates.xF, textureV: textureCoordinates.yF), // B
            Vertex(position: frame.topRight, textureU: textureCoordinates.xF + textureCoordinates.widthF, textureV: textureCoordinates.yF), // C
            Vertex(position: frame.bottomRight, textureU: textureCoordinates.xF + textureCoordinates.widthF, textureV: textureCoordinates.yF + textureCoordinates.heightF), // D
        ]
        let indices: [UInt16] = [
            0, 1, 2, // ABC
            0, 2, 3, // ACD
        ]

        guard let verticesBuffer = device.makeBuffer(bytes: vertecies, length: MemoryLayout<Vertex>.stride * vertecies.count, options: []), let indicesBuffer = device.makeBuffer(bytes: indices, length: MemoryLayout<UInt16>.stride * indices.count, options: []) else {
            fatalError("Cannot allocate buffers")
        }

        lock.withCritical {
            indicesCount = indices.count
            self.verticesBuffer = verticesBuffer
            self.indicesBuffer = indicesBuffer
        }

        createTessellationFactors(patchCount: indices.count)
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

    func loadTexture(_ context: MCRenderingContextInterface?, textureHolder: MCTextureHolderInterface?) {
        guard let textureHolder = textureHolder as? TextureHolder else {
            fatalError("unexpected TextureHolder")
        }
        texture = textureHolder.texture

    }

    func removeTexture() {}

    func asGraphicsObject() -> MCGraphicsObjectInterface? {
        self
    }

    func asMaskingObject() -> MCMaskingObjectInterface? { self }
}

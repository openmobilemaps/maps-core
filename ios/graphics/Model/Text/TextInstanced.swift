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

final class TextInstanced: BaseGraphicsObject {
    private var shader: TextInstancedShader

    private var verticesBuffer: MTLBuffer?
    private var indicesBuffer: MTLBuffer?
    private var indicesCount: Int = 0

    private var instanceCount: Int = 0
    private var positionsBuffer: MTLBuffer?
    private var textureCoordinatesBuffer: MTLBuffer?
    private var scalesBuffer: MTLBuffer?
    private var rotationsBuffer: MTLBuffer?
    private var styleIndicesBuffer: MTLBuffer?
    private var styleBuffer: MTLBuffer?

    private var texture: MTLTexture?

    private var stencilState: MTLDepthStencilState?

    init(shader: MCShaderProgramInterface, metalContext: MetalContext) {
        self.shader = shader as! TextInstancedShader
        super.init(device: metalContext.device,
                   sampler: metalContext.samplerLibrary.value(Sampler.magLinear.rawValue)!,
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
              let indicesBuffer,
              let positionsBuffer,
              let scalesBuffer,
              let rotationsBuffer,
              let textureCoordinatesBuffer,
              let styleIndicesBuffer,
              let styleBuffer,
              instanceCount != 0 else { return }

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
        if let matrixPointer = UnsafeRawPointer(bitPattern: Int(mvpMatrix)) {
            encoder.setVertexBytes(matrixPointer, length: 64, index: 1)
        }

        encoder.setFragmentSamplerState(sampler, index: 0)

        if let texture {
            encoder.setFragmentTexture(texture, index: 0)
        }

        encoder.setVertexBuffer(positionsBuffer, offset: 0, index: 2)
        encoder.setVertexBuffer(scalesBuffer, offset: 0, index: 3)
        encoder.setVertexBuffer(rotationsBuffer, offset: 0, index: 4)
        encoder.setVertexBuffer(textureCoordinatesBuffer, offset: 0, index: 5)
        encoder.setVertexBuffer(styleIndicesBuffer, offset: 0, index: 6)

        encoder.setFragmentBuffer(styleBuffer, offset: 0, index: 1)

        var isHalo: Bool = true

        encoder.setFragmentBytes(&isHalo, length: MemoryLayout<Bool>.stride, index: 2)

        encoder.drawIndexedPrimitives(type: .triangle,
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

        encoder.setFragmentBytes(&isHalo, length: MemoryLayout<Bool>.stride, index: 2)

        encoder.drawIndexedPrimitives(type: .triangle,
                                      indexCount: indicesCount,
                                      indexType: .uint16,
                                      indexBuffer: indicesBuffer,
                                      indexBufferOffset: 0,
                                      instanceCount: instanceCount)
    }
}

extension TextInstanced: MCTextInstancedInterface {
    func setFrame(_ frame: MCQuad2dD) {
        /*
         The quad is made out of 4 vertices as following
         B----C
         |    |
         |    |
         A----D
         Where A-C are joined to form two triangles
         */
        let vertecies: [Vertex] = [
            Vertex(position: frame.bottomLeft, textureU: 0, textureV: 1), // A
            Vertex(position: frame.topLeft, textureU: 0, textureV: 0), // B
            Vertex(position: frame.topRight, textureU: 1, textureV: 0), // C
            Vertex(position: frame.bottomRight, textureU: 1, textureV: 1), // D
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
    }

    func loadTexture(_ context: MCRenderingContextInterface?, textureHolder: MCTextureHolderInterface?) {
        guard let textureHolder = textureHolder as? TextureHolder else {
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

    func setPositions(_ positions: MCSharedBytes) {
        lock.withCritical {
            positionsBuffer.copyOrCreate(from: positions, device: device)
        }
    }

    func setTextureCoordinates(_ textureCoordinates: MCSharedBytes) {
        lock.withCritical {
            textureCoordinatesBuffer.copyOrCreate(from: textureCoordinates, device: device)
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

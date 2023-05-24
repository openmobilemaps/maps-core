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

final class PolygonPatternGroup2d: BaseGraphicsObject {
    private var shader: PolygonPatternGroupShader

    private var verticesBuffer: MTLBuffer?
    private var indicesBuffer: MTLBuffer?
    private var indicesCount: Int = 0

    private var opacitiesBuffer: MTLBuffer?
    private var textureCoordinatesBuffer: MTLBuffer?

    private var stencilState: MTLDepthStencilState?

    private var texture: MTLTexture?

    private var screenPixelFactor: Float = 0
    private var customScreenPixelFactor: Float = 0

    init(shader: MCShaderProgramInterface, metalContext: MetalContext) {
        guard let shader = shader as? PolygonPatternGroupShader else {
            fatalError("PolygonPatternGroup2d only supports PolygonPatternGroupShader")
        }
        self.shader = shader
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

    override func render(encoder: MTLRenderCommandEncoder,
                         context: RenderingContext,
                         renderPass _: MCRenderPassConfig,
                         mvpMatrix: Int64,
                         isMasked: Bool,
                         screenPixelAsRealMeterFactor: Double) {
        lock.lock()
        defer {
            lock.unlock()
        }

        guard let verticesBuffer = verticesBuffer,
              let indicesBuffer = indicesBuffer,
              let opacitiesBuffer = opacitiesBuffer,
              let textureCoordinatesBuffer = textureCoordinatesBuffer,
              let texture = texture else { return }

        #if DEBUG
        encoder.pushDebugGroup("PolygonPatternGroup2d")
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
        }

        shader.setupProgram(context)
        shader.preRender(context)

        encoder.setVertexBuffer(verticesBuffer, offset: 0, index: 0)
        if let matrixPointer = UnsafeRawPointer(bitPattern: Int(mvpMatrix)) {
            encoder.setVertexBytes(matrixPointer, length: 64, index: 1)
        }
        if customScreenPixelFactor != 0 {
            encoder.setVertexBytes(&customScreenPixelFactor, length: MemoryLayout<Float>.stride, index: 2)
        } else {
            screenPixelFactor = Float(screenPixelAsRealMeterFactor)
            encoder.setVertexBytes(&screenPixelFactor, length: MemoryLayout<Float>.stride, index: 2)
        }

        encoder.setFragmentTexture(texture, index: 0)
        encoder.setFragmentSamplerState(sampler, index: 0)

        encoder.setFragmentBuffer(opacitiesBuffer, offset: 0, index: 0)
        encoder.setFragmentBuffer(textureCoordinatesBuffer, offset: 0, index: 1)

        encoder.drawIndexedPrimitives(type: .triangle,
                                      indexCount: indicesCount,
                                      indexType: .uint16,
                                      indexBuffer: indicesBuffer,
                                      indexBufferOffset: 0)

    }
}

extension PolygonPatternGroup2d: MCPolygonPatternGroup2dInterface {
    func setVertices(_ vertices: MCSharedBytes, indices: MCSharedBytes) {
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
        lock.withCritical {
            self.indicesCount = Int(indices.elementCount)
            self.verticesBuffer = verticesBuffer
            self.indicesBuffer = indicesBuffer
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

    func setScalingFactor(_ factor: Float) {
        lock.withCritical {
            customScreenPixelFactor = factor
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

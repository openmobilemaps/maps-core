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

final class Quad2d: BaseGraphicsObject {
    private var verticesBuffer: MTLBuffer?

    private var indicesBuffer: MTLBuffer?

    private var indicesCount: Int = 0

    private var texture: MTLTexture?

    private var shader: MCShaderProgramInterface

    private var stencilState: MTLDepthStencilState?

    private var renderAsMask = false

    init(shader: MCShaderProgramInterface, metalContext: MetalContext, label: String = "Quad2d") {
        self.shader = shader
        super.init(device: metalContext.device,
                   sampler: metalContext.samplerLibrary.value(Sampler.magLinear.rawValue)!,
                   label: label)
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
        if shader is AlphaShader || shader is RasterShader {
            return texture != nil
        }
        return true
    }

    override func render(encoder: MTLRenderCommandEncoder,
                         context: RenderingContext,
                         renderPass _: MCRenderPassConfig,
                         mvpMatrix: Int64,
                         isMasked: Bool,
                         screenPixelAsRealMeterFactor _: Double) {
        guard isReady(),
              let verticesBuffer,
              let indicesBuffer else { return }

        lock.lock()
        defer {
            lock.unlock()
        }
        if shader is AlphaShader || shader is RasterShader, texture == nil {
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
        shader.preRender(context)

        encoder.setVertexBuffer(verticesBuffer, offset: 0, index: 0)
        if let matrixPointer = UnsafeRawPointer(bitPattern: Int(mvpMatrix)) {
            encoder.setVertexBytes(matrixPointer, length: 64, index: 1)
        }

        encoder.setFragmentSamplerState(sampler, index: 0)

        if let texture {
            encoder.setFragmentTexture(texture, index: 0)
        }

        encoder.drawIndexedPrimitives(type: .triangle,
                                      indexCount: indicesCount,
                                      indexType: .uint16,
                                      indexBuffer: indicesBuffer,
                                      indexBufferOffset: 0)
    }
}

extension Quad2d: MCMaskingObjectInterface {
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

extension Quad2d: MCQuad2dInterface {
    func setFrame(_ frame: MCQuad2dD, textureCoordinates: MCRectD) {
        /*
         The quad is made out of 4 vertices as following
         B----C
         |    |
         |    |
         A----D
         Where A-C are joined to form two triangles
         */
        let vertices: [Vertex] = [
            Vertex(position: frame.bottomLeft, textureU: textureCoordinates.xF, textureV: textureCoordinates.yF + textureCoordinates.heightF), // A
            Vertex(position: frame.topLeft, textureU: textureCoordinates.xF, textureV: textureCoordinates.yF), // B
            Vertex(position: frame.topRight, textureU: textureCoordinates.xF + textureCoordinates.widthF, textureV: textureCoordinates.yF), // C
            Vertex(position: frame.bottomRight, textureU: textureCoordinates.xF + textureCoordinates.widthF, textureV: textureCoordinates.yF + textureCoordinates.heightF), // D
        ]
        let indices: [UInt16] = [
            0, 1, 2, // ABC
            0, 2, 3, // ACD
        ]

        lock.withCritical {
            self.verticesBuffer.copyOrCreate(bytes: vertices, length: MemoryLayout<Vertex>.stride * vertices.count, device: device)
            self.indicesBuffer.copyOrCreate(bytes: indices, length: MemoryLayout<UInt16>.stride * indices.count, device: device)
            if self.verticesBuffer != nil, self.indicesBuffer != nil {
                self.indicesCount = indices.count
            } else {
                self.indicesCount = 0
            }
        }
    }

    func loadTexture(_ context: MCRenderingContextInterface?, textureHolder: MCTextureHolderInterface?) {
        guard let textureHolder = textureHolder as? TextureHolder else {
            fatalError("unexpected TextureHolder")
        }
        texture = textureHolder.texture
    }

    func removeTexture() {
    }

    func asGraphicsObject() -> MCGraphicsObjectInterface? {
        self
    }

    func asMaskingObject() -> MCMaskingObjectInterface? { self }
}

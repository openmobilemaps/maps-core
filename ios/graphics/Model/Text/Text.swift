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

class Text: BaseGraphicsObject {
    private var shader: MCShaderProgramInterface

    private var verticesBuffer: MTLBuffer?
    private var indicesBuffer: MTLBuffer?
    private var indicesCount: Int = 0

    private var texture: MTLTexture?

    private var stencilState: MTLDepthStencilState?

    init(shader: MCShaderProgramInterface, metalContext: MetalContext) {
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
                         screenPixelAsRealMeterFactor _: Double)
    {
        guard let verticesBuffer = verticesBuffer,
              let indicesBuffer = indicesBuffer else { return }

        if isMasked {
            if stencilState == nil {
                setupStencilStates()
            }
            encoder.setDepthStencilState(stencilState)
            encoder.setStencilReferenceValue(0b1000_0000)
        }

        encoder.pushDebugGroup("Text")

        shader.setupProgram(context)
        shader.preRender(context)

        encoder.setVertexBuffer(verticesBuffer, offset: 0, index: 0)
        if let matrixPointer = UnsafeRawPointer(bitPattern: Int(mvpMatrix)) {
            encoder.setVertexBytes(matrixPointer, length: 64, index: 1)
        }

        encoder.setFragmentSamplerState(sampler, index: 0)

        if let texture = texture {
            encoder.setFragmentTexture(texture, index: 0)
        }

        encoder.drawIndexedPrimitives(type: .triangle,
                                      indexCount: indicesCount,
                                      indexType: .uint16,
                                      indexBuffer: indicesBuffer,
                                      indexBufferOffset: 0)

        encoder.popDebugGroup()
    }
}

extension Text: MCTextInterface {
    func setTexts(_ texts: [MCTextDescription]) {
        var vertices: [Vertex] = []
        var indices: [UInt16] = []

        var indicesStart: UInt16 = 0
        for t in texts {
            for f in t.glyphs {
                let frame = f.frame
                let textureCoordinates = f.textureCoordinates
                /*
                 The quad is made out of 4 vertices as following
                 B----C
                 |    |
                 |    |
                 A----D
                 Where A-C are joined to form two triangles
                 */
                let v: [Vertex] = [
                    Vertex(position: frame.bottomLeft, textureU: textureCoordinates.bottomLeft.xF, textureV: textureCoordinates.bottomLeft.yF), // A
                    Vertex(position: frame.topLeft, textureU: textureCoordinates.topLeft.xF, textureV: textureCoordinates.topLeft.yF), // B
                    Vertex(position: frame.topRight, textureU: textureCoordinates.topRight.xF, textureV: textureCoordinates.topRight.yF), // C
                    Vertex(position: frame.bottomRight, textureU: textureCoordinates.bottomRight.xF, textureV: textureCoordinates.bottomRight.yF), // D
                ]

                vertices.append(contentsOf: v)

                let i: [UInt16] = [
                    0 + indicesStart, 1 + indicesStart, 2 + indicesStart, // ABC
                    0 + indicesStart, 2 + indicesStart, 3 + indicesStart, // ACD
                ]

                indices.append(contentsOf: i)

                indicesStart += 4
            }
        }

        guard let verticesBuffer = device.makeBuffer(bytes: vertices, length: MemoryLayout<Vertex>.stride * vertices.count, options: []), let indicesBuffer = device.makeBuffer(bytes: indices, length: MemoryLayout<UInt16>.stride * indices.count, options: []) else {
            fatalError("Cannot allocate buffers")
        }

        indicesCount = indices.count
        self.verticesBuffer = verticesBuffer
        self.indicesBuffer = indicesBuffer
    }

    func loadTexture(_ textureHolder: MCTextureHolderInterface?) {
        guard let textureHolder = textureHolder as? TextureHolder else {
            fatalError("unexpected TextureHolder")
        }

        texture = textureHolder.texture
    }

    func removeTexture() {
        texture = nil
    }

    func asGraphicsObject() -> MCGraphicsObjectInterface? {
        self
    }
}

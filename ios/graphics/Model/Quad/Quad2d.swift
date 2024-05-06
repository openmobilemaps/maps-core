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
    private var renderPassStencilState: MTLDepthStencilState?

    private var renderAsMask = false

    private var subdivisionFactor: Int32 = 0

    private var frame: MCQuad3dD?
    private var textureCoordinates: MCRectD?

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
                         renderPass: MCRenderPassConfig,
                         vpMatrix: Int64,
                         mMatrix: Int64,
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
        } else if renderPass.isPassMasked {
            if renderPassStencilState == nil {
                renderPassStencilState = self.renderPassMaskStencilState()
            }

            encoder.setDepthStencilState(renderPassStencilState)
            encoder.setStencilReferenceValue(0b0000_0000)
        } else {
            encoder.setDepthStencilState(context.defaultMask)
        }

        shader.setupProgram(context)
        shader.preRender(context)

        encoder.setVertexBuffer(verticesBuffer, offset: 0, index: 0)
        
        if let vpMatrixPointer = UnsafeRawPointer(bitPattern: Int(vpMatrix)) {
            encoder.setVertexBytes(vpMatrixPointer, length: 64, index: 1)
        }
        if let mMatrixPointer = UnsafeRawPointer(bitPattern: Int(mMatrix)) {
            encoder.setVertexBytes(mMatrixPointer, length: 64, index: 2)
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
                vpMatrix: Int64,
                mMatrix: Int64,
                screenPixelAsRealMeterFactor: Double) {
        guard isReady(),
              let context = context as? RenderingContext,
              let encoder = context.encoder else { return }

        renderAsMask = true

        render(encoder: encoder,
               context: context,
               renderPass: renderPass,
               vpMatrix: vpMatrix,
               mMatrix: mMatrix,
               isMasked: false,
               screenPixelAsRealMeterFactor: screenPixelAsRealMeterFactor)
    }
}

extension Quad2d: MCQuad2dInterface {

    func setSubdivisionFactor(_ factor: Int32) {
        if subdivisionFactor != factor {
            self.subdivisionFactor = factor

            if let frame,
               let textureCoordinates {
                setFrame(frame, textureCoordinates: textureCoordinates)
            }
        }
    }


    func setFrame(_ frame: MCQuad3dD, textureCoordinates: MCRectD) {
        self.frame = frame
        self.textureCoordinates = textureCoordinates

        var vertices: [Vertex3D] = []
        var indices: [UInt16] = []

        if subdivisionFactor == 0 {
            /*
             The quad is made out of 4 vertices as following
             B----C
             |    |
             |    |
             A----D
             Where A-C are joined to form two triangles
             */
            vertices = [
                Vertex3D(position: frame.bottomLeft, textureU: textureCoordinates.xF, textureV: textureCoordinates.yF + textureCoordinates.heightF), // A
                Vertex3D(position: frame.topLeft, textureU: textureCoordinates.xF, textureV: textureCoordinates.yF), // B
                Vertex3D(position: frame.topRight, textureU: textureCoordinates.xF + textureCoordinates.widthF, textureV: textureCoordinates.yF), // C
                Vertex3D(position: frame.bottomRight, textureU: textureCoordinates.xF + textureCoordinates.widthF, textureV: textureCoordinates.yF + textureCoordinates.heightF), // D
            ]
            indices = [
                0, 2, 1, // ACB
                0, 3, 2, // ADC
            ]

        } else {

            let numSubd = Int(pow(2.0, Double(subdivisionFactor)))

            let deltaRTop = MCVec3F(x: Float(frame.topRight.x - frame.topLeft.x),
                                    y: Float(frame.topRight.y - frame.topLeft.y),
                                    z: Float(frame.topRight.z - frame.topLeft.z))
            let deltaDLeft = MCVec3F(x: Float(frame.bottomLeft.x - frame.topLeft.x),
                                     y: Float(frame.bottomLeft.y - frame.topLeft.y),
                                     z: Float(frame.bottomLeft.z - frame.topLeft.z))
            let deltaDRight = MCVec3F(x: Float(frame.bottomRight.x - frame.topRight.x),
                                      y: Float(frame.bottomRight.y - frame.topRight.y),
                                      z: Float(frame.bottomRight.z - frame.topRight.z))

            for iR in 0...numSubd {
                let pcR = Float(iR) / Float(numSubd)
                let originX = frame.topLeft.xF + pcR * deltaRTop.x
                let originY = frame.topLeft.yF + pcR * deltaRTop.y
                let originZ = frame.topLeft.zF + pcR * deltaRTop.z
                for iD in 0...numSubd {
                    let pcD = Float(iD) / Float(numSubd)
                    let deltaDX = pcD * ((1.0 - pcR) * deltaDLeft.x + pcR * deltaDRight.x)
                    let deltaDY = pcD * ((1.0 - pcR) * deltaDLeft.y + pcR * deltaDRight.y)
                    let deltaDZ = pcD * ((1.0 - pcR) * deltaDLeft.z + pcR * deltaDRight.z)

                    let u: Float = Float(textureCoordinates.xF + pcR * textureCoordinates.widthF)
                    let v: Float = Float(textureCoordinates.yF + pcD * textureCoordinates.heightF)

                    vertices.append(Vertex3D(x: originX + deltaDX, y: originY + deltaDY, z: originZ + deltaDZ, textureU: u, textureV: v))

                    if iR < numSubd && iD < numSubd {
                        let baseInd = UInt16(iD + (iR * (numSubd + 1)))
                        let baseIndNextCol = UInt16(baseInd + UInt16(numSubd + 1))
                        indices.append(contentsOf: [baseInd, baseInd + 1, baseIndNextCol + 1, baseInd, baseIndNextCol + 1, baseIndNextCol])
                    }
                }
            }
        }

        guard let verticesBuffer = device.makeBuffer(bytes: vertices, length: MemoryLayout<Vertex3D>.stride * vertices.count, options: []), let indicesBuffer = device.makeBuffer(bytes: indices, length: MemoryLayout<UInt16>.stride * indices.count, options: []) else {
            fatalError("Cannot allocate buffers")
        }

        lock.withCritical {
            self.verticesBuffer.copyOrCreate(bytes: vertices, length: MemoryLayout<Vertex3D>.stride * vertices.count, device: device)
            self.indicesBuffer.copyOrCreate(bytes: indices, length: MemoryLayout<UInt16>.stride * indices.count, device: device)
            if self.verticesBuffer != nil && self.indicesBuffer != nil {
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

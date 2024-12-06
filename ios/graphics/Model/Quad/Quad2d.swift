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
import simd

final class Quad2d: BaseGraphicsObject, @unchecked Sendable {
    private var verticesBuffer: MTLBuffer?

    private var indicesBuffer: MTLBuffer?
    private var is3d = false

    private var indicesCount: Int = 0

    private var texture: MTLTexture?

    private var shader: MCShaderProgramInterface

    private var stencilState: MTLDepthStencilState?
    private var renderPassStencilState: MTLDepthStencilState?

    private var renderAsMask = false

    private var subdivisionFactor: Int32 = 0

    private var frame: MCQuad3dD?
    private var textureCoordinates: MCRectD?

    init(
        shader: MCShaderProgramInterface, metalContext: MetalContext,
        label: String = "Quad2d"
    ) {
        self.shader = shader
        super.init(
            device: metalContext.device,
            sampler: metalContext.samplerLibrary.value(
                Sampler.magLinear.rawValue)!,
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
        guard ready else {
            return false
        }
        if shader is AlphaShader || shader is RasterShader {
            return texture != nil
        }
        return true
    }

    override func render(
        encoder: MTLRenderCommandEncoder,
        context: RenderingContext,
        renderPass: MCRenderPassConfig,
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

        guard isReady(),
            let verticesBuffer,
            let indicesBuffer
        else { return }

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

        let vpMatrixBuffer = vpMatrixBuffers.getNextBuffer(context)
        if let matrixPointer = UnsafeRawPointer(bitPattern: Int(vpMatrix)) {
            vpMatrixBuffer?.contents().copyMemory(
                from: matrixPointer, byteCount: 64)
        }
        encoder.setVertexBuffer(vpMatrixBuffer, offset: 0, index: 1)

        if shader.usesModelMatrix() {
            if let matrixPointer = UnsafeRawPointer(bitPattern: Int(mMatrix)) {
                let mMatrixBuffer = device.makeBuffer(bytes: matrixPointer, length: 64)
                encoder.setVertexBuffer(mMatrixBuffer, offset: 0, index: 2)
            }
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
        encoder.setVertexBuffer(originOffsetBuffer, offset: 0, index: 3)

        encoder.setFragmentSamplerState(sampler, index: 0)

        if let texture {
            encoder.setFragmentTexture(texture, index: 0)
        }

        encoder.drawIndexedPrimitives(
            type: .triangle,
            indexCount: indicesCount,
            indexType: .uint16,
            indexBuffer: indicesBuffer,
            indexBufferOffset: 0)
    }
}

extension Quad2d: MCMaskingObjectInterface {
    func render(
        asMask context: MCRenderingContextInterface?,
        renderPass: MCRenderPassConfig,
        vpMatrix: Int64,
        mMatrix: Int64,
        origin: MCVec3D,
        screenPixelAsRealMeterFactor: Double
    ) {
        guard isReady(),
            let context = context as? RenderingContext,
            let encoder = context.encoder
        else { return }

        renderAsMask = true

        render(
            encoder: encoder,
            context: context,
            renderPass: renderPass,
            vpMatrix: vpMatrix,
            mMatrix: mMatrix,
            origin: origin,
            isMasked: false,
            screenPixelAsRealMeterFactor: screenPixelAsRealMeterFactor)
    }
}

extension Quad2d: MCQuad2dInterface {

    func setSubdivisionFactor(_ factor: Int32) {
        let (optFrame, optTextureCoordinates) = lock.withCritical {
            () -> (MCQuad3dD?, MCRectD?) in
            if self.subdivisionFactor != factor {
                self.subdivisionFactor = factor
                return (frame, textureCoordinates)
            } else {
                return (nil, nil)
            }
        }
        if let frame = optFrame,
            let textureCoordinates = optTextureCoordinates
        {
            setFrame(
                frame, textureCoordinates: textureCoordinates,
                origin: self.originOffset, is3d: is3d)
        }
    }

    func setFrame(
        _ frame: MCQuad3dD, textureCoordinates: MCRectD, origin: MCVec3D,
        is3d: Bool
    ) {
        var vertices: [Vertex3DTexture] = []
        var indices: [UInt16] = []

        let sFactor = lock.withCritical { subdivisionFactor }

        func transform(_ coordinate: MCVec3D) -> MCVec3D {
            if is3d {
                let x = 1.0 * sin(coordinate.y) * cos(coordinate.x) - origin.x
                let y = 1.0 * cos(coordinate.y) - origin.y
                let z = -1.0 * sin(coordinate.y) * sin(coordinate.x) - origin.z
                return MCVec3D(x: x, y: y, z: z)
            } else {
                let x = coordinate.x - origin.x
                let y = coordinate.y - origin.y
                let z = coordinate.z - origin.z
                return MCVec3D(x: x, y: y, z: z)
            }
        }

        if sFactor == 0 {
            /*
             The quad is made out of 4 vertices as following
             B----C
             |    |
             |    |
             A----D
             Where A-C are joined to form two triangles
             */
            vertices = [
                Vertex3DTexture(
                    position: transform(frame.bottomLeft),
                    textureU: textureCoordinates.xF,
                    textureV: textureCoordinates.yF + textureCoordinates.heightF
                ),  // A
                Vertex3DTexture(
                    position: transform(frame.topLeft),
                    textureU: textureCoordinates.xF,
                    textureV: textureCoordinates.yF),  // B
                Vertex3DTexture(
                    position: transform(frame.topRight),
                    textureU: textureCoordinates.xF + textureCoordinates.widthF,
                    textureV: textureCoordinates.yF),  // C
                Vertex3DTexture(
                    position: transform(frame.bottomRight),
                    textureU: textureCoordinates.xF + textureCoordinates.widthF,
                    textureV: textureCoordinates.yF + textureCoordinates.heightF
                ),  // D
            ]
            indices = [
                0, 2, 1,  // ACB
                0, 3, 2,  // ADC
            ]

        } else {

            let numSubd = Int(pow(2.0, Double(sFactor)))

            let deltaRTop = MCVec3D(
                x: Double(frame.topRight.x - frame.topLeft.x),
                y: Double(frame.topRight.y - frame.topLeft.y),
                z: Double(frame.topRight.z - frame.topLeft.z))
            let deltaDLeft = MCVec3D(
                x: Double(frame.bottomLeft.x - frame.topLeft.x),
                y: Double(frame.bottomLeft.y - frame.topLeft.y),
                z: Double(frame.bottomLeft.z - frame.topLeft.z))
            let deltaDRight = MCVec3D(
                x: Double(frame.bottomRight.x - frame.topRight.x),
                y: Double(frame.bottomRight.y - frame.topRight.y),
                z: Double(frame.bottomRight.z - frame.topRight.z))

            for iR in 0...numSubd {
                let pcR = Double(iR) / Double(numSubd)
                let originX = frame.topLeft.x + pcR * deltaRTop.x
                let originY = frame.topLeft.y + pcR * deltaRTop.y
                let originZ = frame.topLeft.z + pcR * deltaRTop.z
                for iD in 0...numSubd {
                    let pcD = Double(iD) / Double(numSubd)
                    let deltaDX =
                        pcD * ((1.0 - pcR) * deltaDLeft.x + pcR * deltaDRight.x)
                    let deltaDY =
                        pcD * ((1.0 - pcR) * deltaDLeft.y + pcR * deltaDRight.y)
                    let deltaDZ =
                        pcD * ((1.0 - pcR) * deltaDLeft.z + pcR * deltaDRight.z)

                    let u: Float = Float(
                        textureCoordinates.x + pcR * textureCoordinates.width)
                    let v: Float = Float(
                        textureCoordinates.y + pcD * textureCoordinates.height)

                    vertices.append(
                        Vertex3DTexture(
                            position: transform(
                                .init(
                                    x: originX + deltaDX, y: originY + deltaDY,
                                    z: originZ + deltaDZ)), textureU: u,
                            textureV: v))

                    if iR < numSubd && iD < numSubd {
                        let baseInd = UInt16(iD + (iR * (numSubd + 1)))
                        let baseIndNextCol = UInt16(
                            baseInd + UInt16(numSubd + 1))
                        indices.append(contentsOf: [
                            baseInd, baseInd + 1, baseIndNextCol + 1, baseInd,
                            baseIndNextCol + 1, baseIndNextCol,
                        ])
                    }
                }
            }
        }

        lock.withCritical {
            self.is3d = is3d
            self.originOffset = origin
            self.frame = frame
            self.textureCoordinates = textureCoordinates
            self.verticesBuffer.copyOrCreate(
                bytes: vertices,
                length: MemoryLayout<Vertex3DTexture>.stride * vertices.count,
                device: device)
            self.indicesBuffer.copyOrCreate(
                bytes: indices,
                length: MemoryLayout<UInt16>.stride * indices.count,
                device: device)
            if self.verticesBuffer != nil, self.indicesBuffer != nil {
                self.indicesCount = indices.count
                assert(
                    self.indicesCount * 2 == MemoryLayout<UInt16>.stride
                        * indices.count)
            } else {
                self.indicesCount = 0
            }
        }
    }

    func loadTexture(
        _ context: MCRenderingContextInterface?,
        textureHolder: MCTextureHolderInterface?
    ) {
        guard let textureHolder = textureHolder as? TextureHolder else {
            fatalError("unexpected TextureHolder")
        }
        lock.withCritical {
            texture = textureHolder.texture
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

    func asMaskingObject() -> MCMaskingObjectInterface? { self }
}

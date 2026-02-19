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
import UIKit
import simd

final class Quad2dTessellatedDisplaced: BaseGraphicsObject, @unchecked Sendable {
    private var verticesBuffer: MTLBuffer?
    
    private var tessellationFactorsBuffer: MTLBuffer?
    private var originBuffers: MultiBuffer<simd_float4>
    
    private var indicesBuffer: MTLBuffer?
    private var indicesCount: Int = 0
    
    private var is3d = false
    private var subdivisionFactor: Int32 = 0

    private var texture: MTLTexture?
    private var elevationTexture: MTLTexture?

    private var shader: MCShaderProgramInterface

    private var maskedDepthStencilState: MTLDepthStencilState?
    private var renderAsMaskDepthStencilState: MTLDepthStencilState?
    private var renderPassDepthStencilState: MTLDepthStencilState?
    private var defaultDepthStencilState: MTLDepthStencilState?

    private var renderAsMask = false

    private var frame: MCQuad3dD?
    private var textureCoordinates: MCRectD?

    private var nearestSampler: MTLSamplerState

    private var samplerToUse = Sampler.magLinear

    init(
        shader: MCShaderProgramInterface, metalContext: MetalContext,
        label: String = "Quad2dTessellated"
    ) {
        self.shader = shader
        originBuffers = .init(device: metalContext.device)
        nearestSampler = metalContext.samplerLibrary.value(
            Sampler.magNearest.rawValue)!
        super
            .init(
                device: metalContext.device,
                sampler: metalContext.samplerLibrary.value(
                    Sampler.magLinear.rawValue)!,
                label: label)
        
        
        let factorH = Half(pow(2, Float(self.subdivisionFactor))).bits;
        
        var tessellationFactors = MTLQuadTessellationFactorsHalf(
            edgeTessellationFactor: (factorH, factorH, factorH, factorH),
            insideTessellationFactor: (factorH, factorH)
        );
            
        self.tessellationFactorsBuffer.copyOrCreate(
            bytes: &tessellationFactors,
            length: MemoryLayout<MTLQuadTessellationFactorsHalf>.stride,
            device: device)
    }

    private func setupDepthStencilStates() {
        let maskedStencil = MTLStencilDescriptor()
        maskedStencil.stencilCompareFunction = .equal
        maskedStencil.stencilFailureOperation = .zero
        maskedStencil.depthFailureOperation = .keep
        maskedStencil.depthStencilPassOperation = .keep
        maskedStencil.readMask = 0b1111_1111
        maskedStencil.writeMask = 0b0000_0000
        maskedDepthStencilState = makeDepthStencilState(stencil: maskedStencil)

        let renderAsMaskStencil = MTLStencilDescriptor()
        renderAsMaskStencil.stencilCompareFunction = .always
        renderAsMaskStencil.stencilFailureOperation = .keep
        renderAsMaskStencil.depthFailureOperation = .keep
        renderAsMaskStencil.depthStencilPassOperation = .replace
        renderAsMaskStencil.writeMask = 0b1100_0000
        renderAsMaskDepthStencilState = makeDepthStencilState(stencil: renderAsMaskStencil)

        let renderPassStencil = MTLStencilDescriptor()
        renderPassStencil.stencilCompareFunction = .equal
        renderPassStencil.stencilFailureOperation = .keep
        renderPassStencil.depthFailureOperation = .keep
        renderPassStencil.depthStencilPassOperation = .incrementWrap
        renderPassStencil.readMask = 0b1111_1111
        renderPassStencil.writeMask = 0b0000_0001
        renderPassDepthStencilState = makeDepthStencilState(stencil: renderPassStencil)

        let defaultStencil = MTLStencilDescriptor()
        defaultStencil.stencilCompareFunction = .equal
        defaultStencil.depthStencilPassOperation = .keep
        defaultDepthStencilState = makeDepthStencilState(stencil: defaultStencil)
    }

    private func makeDepthStencilState(stencil: MTLStencilDescriptor) -> MTLDepthStencilState? {
        let descriptor = MTLDepthStencilDescriptor()
        //descriptor.depthCompareFunction = .lessEqual
        descriptor.isDepthWriteEnabled = true
        descriptor.frontFaceStencil = stencil
        descriptor.backFaceStencil = stencil
        return device.makeDepthStencilState(descriptor: descriptor)
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
        screenPixelAsRealMeterFactor _: Double,
        isScreenSpaceCoords: Bool
    ) {
        lock.lock()
        defer {
            lock.unlock()
        }

        guard isReady(),
            let verticesBuffer,
            let tessellationFactorsBuffer,
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

        if maskedDepthStencilState == nil {
            setupDepthStencilStates()
        }

        if isMasked {
            encoder.setDepthStencilState(maskedDepthStencilState)
            encoder.setStencilReferenceValue(0b1100_0000)
        } else if renderAsMask {
            encoder.setDepthStencilState(renderAsMaskDepthStencilState)
            encoder.setStencilReferenceValue(0b1100_0000)
        } else if renderPass.isPassMasked {
            encoder.setDepthStencilState(renderPassDepthStencilState)
            encoder.setStencilReferenceValue(0b0000_0000)
        } else {
            encoder.setDepthStencilState(defaultDepthStencilState)
        }

        shader.setupProgram(context)
        shader.preRender(context, isScreenSpaceCoords: isScreenSpaceCoords)

        encoder.setVertexBuffer(verticesBuffer, offset: 0, index: 0)

        let vpMatrixBuffer = vpMatrixBuffers.getNextBuffer(context)
        if let matrixPointer = UnsafeRawPointer(bitPattern: Int(vpMatrix)) {
            vpMatrixBuffer?.contents()
                .copyMemory(
                    from: matrixPointer, byteCount: 64)
        }
        encoder.setVertexBuffer(vpMatrixBuffer, offset: 0, index: 1)

        if shader.usesModelMatrix() {
            if let mMatrixPointer = UnsafeRawPointer(bitPattern: Int(mMatrix)) {
                encoder.setVertexBytes(mMatrixPointer, length: 64, index: 2)
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

        if samplerToUse == .magNearest {
            encoder.setFragmentSamplerState(nearestSampler, index: 0)
            encoder.setVertexSamplerState(nearestSampler, index: 0)
        } else {
            encoder.setFragmentSamplerState(sampler, index: 0)
            encoder.setVertexSamplerState(sampler, index: 0)
        }

        if let texture {
            encoder.setFragmentTexture(texture, index: 0)
        }
        
        if let elevationTexture {
            encoder.setVertexTexture(elevationTexture, index: 0)
        }
        
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
        encoder.setVertexBuffer(originBuffer, offset: 0, index: 4)
        
        encoder.setVertexBytes(&self.is3d, length: MemoryLayout<Bool>.stride, index: 5)
        
        encoder.setTessellationFactorBuffer(tessellationFactorsBuffer, offset: 0, instanceStride: 0)
        
        #if HARDWARE_TESSELLATION_WIREFRAME_METAL
        let wireframePipeline = MetalContext.current.pipelineLibrary.value(
            Pipeline(
                type: .quadTessellatedWireframeShader,
                blendMode: (shader as? BaseShader)?.blendMode ?? .NORMAL)
        )
        if let wireframePipeline {
            context.setRenderPipelineStateIfNeeded(wireframePipeline)
        }
        encoder.setTriangleFillMode(.lines)
        encoder.drawPatches(
            numberOfPatchControlPoints: 4,
            patchStart: 0,
            patchCount: 1,
            patchIndexBuffer: nil,
            patchIndexBufferOffset: 0,
            instanceCount: 1,
            baseInstance: 0)
        encoder.setTriangleFillMode(.fill)
        shader.preRender(context, isScreenSpaceCoords: isScreenSpaceCoords)
        #endif
        
        encoder.drawIndexedPrimitives(
            type: .triangle,
            indexCount: indicesCount,
            indexType: .uint16,
            indexBuffer: indicesBuffer,
            indexBufferOffset: 0)
    }
}

extension Quad2dTessellatedDisplaced: MCMaskingObjectInterface {
    func render(
        asMask context: MCRenderingContextInterface?,
        renderPass: MCRenderPassConfig,
        vpMatrix: Int64,
        mMatrix: Int64,
        origin: MCVec3D,
        screenPixelAsRealMeterFactor: Double,
        isScreenSpaceCoords: Bool
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
            screenPixelAsRealMeterFactor: screenPixelAsRealMeterFactor,
            isScreenSpaceCoords: isScreenSpaceCoords)
    }
}

extension Quad2dTessellatedDisplaced: MCQuad2dInterface {
    func setMinMagFilter(_ filterType: MCTextureFilterType) {
        switch filterType {
            case .NEAREST:
                samplerToUse = .magNearest
            case .LINEAR:
                samplerToUse = .magLinear
            default:
                break
        }
    }

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
        _ frame: MCQuad3dD, textureCoordinates: MCRectD, origin: MCVec3D, is3d: Bool
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
                return MCVec3D(x: x, y: y, z: 0)
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
        textureHolder: MCTextureHolderInterface?,
        elevationHolder: MCTextureHolderInterface?
    ) {
        guard let textureHolder = textureHolder as? TextureHolder else {
            fatalError("unexpected TextureHolder")
        }
        let elevationHolder = elevationHolder as? TextureHolder
        lock.withCritical {
            texture = textureHolder.texture
            elevationTexture = elevationHolder?.texture
        }
    }

    func removeTexture() {
        lock.withCritical {
            texture = nil
            elevationTexture = nil
        }
    }

    func asGraphicsObject() -> MCGraphicsObjectInterface? {
        self
    }

    func asMaskingObject() -> MCMaskingObjectInterface? { self }
}

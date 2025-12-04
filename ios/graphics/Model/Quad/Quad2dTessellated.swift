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

final class Quad2dTessellated: BaseGraphicsObject, @unchecked Sendable {
    private var verticesBuffer: MTLBuffer?
    
    private var tessellationFactorsBuffer: MTLBuffer?
    private var originBuffers: MultiBuffer<simd_float4>
    
    private var is3d = false

    private var texture: MTLTexture?

    private var shader: MCShaderProgramInterface

    private var stencilState: MTLDepthStencilState?
    private var renderPassStencilState: MTLDepthStencilState?

    private var renderAsMask = false

    private var subdivisionFactor: Int32 = 0

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
        screenPixelAsRealMeterFactor _: Double,
        isScreenSpaceCoords: Bool
    ) {
        lock.lock()
        defer {
            lock.unlock()
        }

        guard isReady(),
            let verticesBuffer,
            let tessellationFactorsBuffer
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
        } else {
            encoder.setFragmentSamplerState(sampler, index: 0)
        }

        if let texture {
            encoder.setFragmentTexture(texture, index: 0)
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
        
        /* ELEVATION PROTOTYPE TEST
        if samplerToUse == .magNearest {
            encoder.setVertexSamplerState(nearestSampler, index: 0)
        } else {
            encoder.setVertexSamplerState(sampler, index: 0)
        }
         
        if let texture {
            encoder.setVertexTexture(texture, index: 0)
        }
        */
        
        encoder.setVertexBytes(&self.is3d, length: MemoryLayout<Bool>.stride, index: 5)
        
        encoder.setTessellationFactorBuffer(tessellationFactorsBuffer, offset: 0, instanceStride: 0)
        
        encoder.drawPatches(
            numberOfPatchControlPoints: 4,
            patchStart: 0,
            patchCount: 1,
            patchIndexBuffer: nil,
            patchIndexBufferOffset: 0,
            instanceCount: 1,
            baseInstance: 0)
    }
}

extension Quad2dTessellated: MCMaskingObjectInterface {
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

extension Quad2dTessellated: MCQuad2dInterface {
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
        _ frame: MCQuad3dD, textureCoordinates: MCRectD, origin: MCVec3D,
        is3d: Bool // maybe move subdivision factor also here? or both own setter? same as in polygon 2d tessellation
    ) {
        let factor = Half(pow(2, Float(lock.withCritical { subdivisionFactor }))).bits;
        
        var tessellationFactors = MTLQuadTessellationFactorsHalf(
            edgeTessellationFactor: (factor, factor, factor, factor),
            insideTessellationFactor: (factor, factor)
        );
        
        var vertices: [TessellatedVertex3DTexture] = []

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
        
        /*
         The quad is made out of 4 vertices as following
         B----C
         |    |
         |    |
         A----D
         Where A-C are joined to form two triangles
         */
        vertices = [
            TessellatedVertex3DTexture(
                relativePosition: transform(frame.topLeft),
                absolutePositionX: frame.topLeft.xF,
                absolutePositionY: frame.topLeft.yF,
                textureU: textureCoordinates.xF,
                textureV: textureCoordinates.yF),  // B
            TessellatedVertex3DTexture(
                relativePosition: transform(frame.topRight),
                absolutePositionX: frame.topRight.xF,
                absolutePositionY: frame.topRight.yF,
                textureU: textureCoordinates.xF + textureCoordinates.widthF,
                textureV: textureCoordinates.yF),  // C
            TessellatedVertex3DTexture(
                relativePosition: transform(frame.bottomLeft),
                absolutePositionX: frame.bottomLeft.xF,
                absolutePositionY: frame.bottomLeft.yF,
                textureU: textureCoordinates.xF,
                textureV: textureCoordinates.yF + textureCoordinates.heightF),  // A
            TessellatedVertex3DTexture(
                relativePosition: transform(frame.bottomRight),
                absolutePositionX: frame.bottomRight.xF,
                absolutePositionY: frame.bottomRight.yF,
                textureU: textureCoordinates.xF + textureCoordinates.widthF,
                textureV: textureCoordinates.yF + textureCoordinates.heightF),  // D
        ]
    
        lock.withCritical {
            self.is3d = is3d
            self.originOffset = origin
            self.frame = frame
            self.textureCoordinates = textureCoordinates
            self.verticesBuffer.copyOrCreate(
                bytes: vertices,
                length: MemoryLayout<TessellatedVertex3DTexture>.stride * vertices.count,
                device: device)
            self.tessellationFactorsBuffer.copyOrCreate(
                bytes: &tessellationFactors,
                length: MemoryLayout<MTLQuadTessellationFactorsHalf>.stride,
                device: device)
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

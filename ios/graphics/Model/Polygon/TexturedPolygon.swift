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
import simd

final class TexturedPolygon: BaseGraphicsObject, @unchecked Sendable {
    private var verticesBuffer: MTLBuffer?
    private var indicesBuffer: MTLBuffer?
    private var indicesCount = 0
    private var tessellationPatchCount = 0

    private var tessellationFactorsBuffer: MTLBuffer?
    private var is3d = false
    private var subdivisionFactor: Int32 = 0

    private var texture: MTLTexture?
    private var elevationTexture: MTLTexture?
    private let shader: MCShaderProgramInterface

    private var stencilState: MTLDepthStencilState?
    private var renderPassStencilState: MTLDepthStencilState?
    private var depthStencilState: MTLDepthStencilState?
    private var renderPassDepthStencilState: MTLDepthStencilState?
    private var renderAsMask = false

    private let nearestSampler: MTLSamplerState
    private var samplerToUse = Sampler.magLinear

    init(shader: MCShaderProgramInterface, metalContext: MetalContext, label: String = "TexturedPolygon") {
        self.shader = shader
        nearestSampler = metalContext.samplerLibrary.value(Sampler.magNearest.rawValue)!
        super
            .init(
                device: metalContext.device,
                sampler: metalContext.samplerLibrary.value(Sampler.magLinear.rawValue)!,
                label: label)

        updateTessellationFactors()
    }

    private var usesTessellation: Bool {
        guard let baseShader = shader as? BaseShader else {
            return false
        }
        return baseShader.shader == .quadTessellatedShader
            || baseShader.shader == .texturedPolygonTessellatedShader
            || baseShader.shader == .texturedPolygonTessellatedDisplacedShader
    }

    private func updateTessellationFactors() {
        let factorH = Half(pow(2, Float(subdivisionFactor))).bits
        var tessellationFactors = MTLTriangleTessellationFactorsHalf(
            edgeTessellationFactor: (factorH, factorH, factorH),
            insideTessellationFactor: factorH
        )

        tessellationFactorsBuffer.copyOrCreate(
            bytes: &tessellationFactors,
            length: MemoryLayout<MTLTriangleTessellationFactorsHalf>.stride,
            device: device)
    }

    private func setupStencilStates() {
        let stencil = MTLStencilDescriptor()
        stencil.stencilCompareFunction = .equal
        stencil.stencilFailureOperation = .zero
        stencil.depthFailureOperation = .keep
        stencil.depthStencilPassOperation = .keep
        stencil.readMask = 0b1111_1111
        stencil.writeMask = 0b0000_0000

        let descriptor = MTLDepthStencilDescriptor()
        descriptor.frontFaceStencil = stencil
        descriptor.backFaceStencil = stencil

        stencilState = device.makeDepthStencilState(descriptor: descriptor)
    }

    private func setupDepthStencilState() {
        let stencil = MTLStencilDescriptor()
        stencil.stencilCompareFunction = .equal
        stencil.stencilFailureOperation = .zero
        stencil.depthFailureOperation = .keep
        stencil.depthStencilPassOperation = .keep
        stencil.readMask = 0b1111_1111
        stencil.writeMask = 0b0000_0000

        let descriptor = MTLDepthStencilDescriptor()
        descriptor.depthCompareFunction = .lessEqual
        descriptor.isDepthWriteEnabled = true
        descriptor.frontFaceStencil = stencil
        descriptor.backFaceStencil = stencil

        depthStencilState = device.makeDepthStencilState(descriptor: descriptor)
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
            let indicesBuffer,
            indicesCount > 0
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

        let usesDepth = elevationTexture != nil

        if isMasked {
            if usesDepth {
                if depthStencilState == nil {
                    setupDepthStencilState()
                }
                encoder.setDepthStencilState(depthStencilState)
            } else {
                if stencilState == nil {
                    setupStencilStates()
                }
                encoder.setDepthStencilState(stencilState)
            }
            encoder.setStencilReferenceValue(0b1100_0000)
        } else if let mask = context.mask, renderAsMask {
            encoder.setDepthStencilState(mask)
            encoder.setStencilReferenceValue(0b1100_0000)
        } else if renderPass.isPassMasked {
            if usesDepth {
                if renderPassDepthStencilState == nil {
                    renderPassDepthStencilState = self.renderPassMaskDepthStencilState()
                }
                encoder.setDepthStencilState(renderPassDepthStencilState)
            } else {
                if renderPassStencilState == nil {
                    renderPassStencilState = self.renderPassMaskStencilState()
                }
                encoder.setDepthStencilState(renderPassStencilState)
            }
            encoder.setStencilReferenceValue(0b0000_0000)
        } else {
            encoder.setDepthStencilState(usesDepth ? context.defaultMaskDepth : context.defaultMask)
        }

        if usesTessellation, let baseShader = shader as? BaseShader {
            let pipelineType: PipelineType =
                elevationTexture == nil
                ? .texturedPolygonTessellatedShader
                : .texturedPolygonTessellatedDisplacedShader
            baseShader.pipeline = MetalContext.current.pipelineLibrary.value(
                Pipeline(type: pipelineType, blendMode: baseShader.blendMode))
        } else {
            shader.setupProgram(context)
        }
        shader.preRender(context, isScreenSpaceCoords: isScreenSpaceCoords)

        encoder.setVertexBuffer(verticesBuffer, offset: 0, index: 0)

        if shader.usesModelMatrix() {
            if let mMatrixPointer = UnsafeRawPointer(bitPattern: Int(mMatrix)) {
                encoder.setVertexBytes(mMatrixPointer, length: 64, index: 2)
            }
        }

        let originOffsetBuffer = originOffsetBuffers.getNextBuffer(context)
        if let bufferPointer = originOffsetBuffer?.contents().assumingMemoryBound(to: simd_float4.self) {
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

        if usesTessellation {
            guard let tessellationFactorsBuffer else {
                return
            }
            encoder.setVertexBytes(&self.is3d, length: MemoryLayout<Bool>.stride, index: 5)
            var hasElevationTexture = elevationTexture != nil
            encoder.setVertexBytes(&hasElevationTexture, length: MemoryLayout<Bool>.stride, index: 6)
            encoder.setTessellationFactorBuffer(tessellationFactorsBuffer, offset: 0, instanceStride: 0)
            if hasElevationTexture {
                encoder.setCullMode(.none)
            }
            encoder.drawIndexedPatches(
                numberOfPatchControlPoints: 3,
                patchStart: 0,
                patchCount: tessellationPatchCount,
                patchIndexBuffer: nil,
                patchIndexBufferOffset: 0,
                controlPointIndexBuffer: indicesBuffer,
                controlPointIndexBufferOffset: 0,
                instanceCount: 1,
                baseInstance: 0)
            if hasElevationTexture {
                restoreCullMode(context.cullMode, encoder: encoder)
            }
        } else {
            encoder.drawIndexedPrimitives(
                type: .triangle,
                indexCount: indicesCount,
                indexType: .uint16,
                indexBuffer: indicesBuffer,
                indexBufferOffset: 0)
        }
    }

    private func restoreCullMode(_ cullMode: MCRenderingCullMode?, encoder: MTLRenderCommandEncoder) {
        guard let cullMode else {
            encoder.setCullMode(.none)
            return
        }

        switch cullMode {
            case .BACK:
                encoder.setCullMode(.front)
            case .FRONT:
                encoder.setCullMode(.back)
            case .NONE:
                encoder.setCullMode(.none)
            @unknown default:
                encoder.setCullMode(.none)
        }
    }
}

extension TexturedPolygon: MCMaskingObjectInterface {
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

extension TexturedPolygon: MCTexturedPolygonInterface {
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
        lock.withCritical {
            if self.subdivisionFactor != factor {
                self.subdivisionFactor = factor
                self.updateTessellationFactors()
            }
        }
    }

    func setVertices(_ vertices: MCSharedBytes, indices: MCSharedBytes, origin: MCVec3D, is3d: Bool) {
        lock.withCritical {
            self.is3d = is3d
            self.verticesBuffer.copyOrCreate(from: vertices, device: device)
            self.indicesBuffer.copyOrCreate(from: indices, device: device)
            if self.verticesBuffer != nil, self.indicesBuffer != nil {
                self.indicesCount = Int(indices.elementCount)
                self.tessellationPatchCount = usesTessellation ? Int(indices.elementCount) / 3 : 0
            } else {
                self.indicesCount = 0
                self.tessellationPatchCount = 0
            }
            self.originOffset = origin
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

    func loadDualTexture(
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

}

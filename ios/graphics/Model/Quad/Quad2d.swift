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

final class Quad2d: BaseGraphicsObject, @unchecked Sendable {
    // --- Buffers for standard rendering ---
    private var verticesBuffer: MTLBuffer?
    private var indicesBuffer: MTLBuffer?
    private var indicesCount: Int = 0

    // --- Buffers and states for tessellation ---
    private var tessellationRenderState2D: MTLRenderPipelineState
    private var tessellationRenderState3D: MTLRenderPipelineState
    private var tessellationFactorBuffer: MTLBuffer?
    private var controlPointBuffer: MTLBuffer?
    private var originBuffer: MTLBuffer?  // For 3D sphere origin

    // --- Common Properties ---
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
        label: String = "Quad2d"
    ) {
        self.shader = shader
        nearestSampler = metalContext.samplerLibrary.value(
            Sampler.magNearest.rawValue)!

        guard
            let baseShader = shader as? BaseShader,
            let state2D = metalContext.pipelineLibrary[Pipeline(type: .tessellatedQuad, blendMode: baseShader.blendMode)],
            let state3D = metalContext.pipelineLibrary[Pipeline(type: .tessellatedQuad3D, blendMode: baseShader.blendMode)]
        else {
            fatalError("Could not create required tessellation pipeline states for Quad2d. Check shader names and library setup.")
        }
        self.tessellationRenderState2D = state2D
        self.tessellationRenderState3D = state3D

        super
            .init(
                device: metalContext.device,
                sampler: metalContext.samplerLibrary.value(Sampler.magLinear.rawValue)!,
                label: label
            )

        self.tessellationFactorBuffer = metalContext.device.makeBuffer(length: MemoryLayout<MTLQuadTessellationFactorsHalf>.size, options: .storageModeShared)
        self.tessellationFactorBuffer?.label = "TessellationFactorBuffer"

        self.originBuffer = metalContext.device.makeBuffer(length: MemoryLayout<SIMD4<Float>>.stride, options: .storageModeShared)
        self.originBuffer?.label = "OriginBuffer"
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

    private func updateTessellationFactors() {
        let sFactor = self.subdivisionFactor  // No lock needed, called from within a locked context or init
        if sFactor <= 0 { return }

        let level = pow(2.0, Float(sFactor))

        guard let tessellationFactorBuffer = self.tessellationFactorBuffer else { return }
        let factors = tessellationFactorBuffer.contents().bindMemory(to: MTLQuadTessellationFactorsHalf.self, capacity: 1)

        // 1. Convert the Float level to a Float16.
        let level16 = Float16(level)

        // 2. Get the raw bit pattern of the Float16, which is a UInt16.
        //    This is the format the GPU hardware expects.
        let levelBits = level16.bitPattern

        // 3. Assign the UInt16 bit pattern to the struct members.
        factors.pointee.edgeTessellationFactor.0 = levelBits
        factors.pointee.edgeTessellationFactor.1 = levelBits
        factors.pointee.edgeTessellationFactor.2 = levelBits
        factors.pointee.edgeTessellationFactor.3 = levelBits

        factors.pointee.insideTessellationFactor.0 = levelBits
        factors.pointee.insideTessellationFactor.1 = levelBits
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
            subdivisionFactor > 0 ? (controlPointBuffer != nil && tessellationFactorBuffer != nil) : (verticesBuffer != nil && indicesBuffer != nil)
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

        // --- Stencil Setup (same for both paths) ---
        if isMasked {
            if stencilState == nil { setupStencilStates() }
            encoder.setDepthStencilState(stencilState)
            encoder.setStencilReferenceValue(0b1100_0000)
        } else if let mask = context.mask, renderAsMask {
            encoder.setDepthStencilState(mask)
            encoder.setStencilReferenceValue(0b1100_0000)
        } else if renderPass.isPassMasked {
            if renderPassStencilState == nil { renderPassStencilState = self.renderPassMaskStencilState() }
            encoder.setDepthStencilState(renderPassStencilState)
            encoder.setStencilReferenceValue(0b0000_0000)
        } else {
            encoder.setDepthStencilState(context.defaultMask)
        }

        // --- Uniforms and Fragment data (same for both paths) ---
        shader.setupProgram(context)
        shader.preRender(context)

        let vpMatrixBuffer = vpMatrixBuffers.getNextBuffer(context)
        if let matrixPointer = UnsafeRawPointer(bitPattern: Int(vpMatrix)) {
            vpMatrixBuffer?.contents().copyMemory(from: matrixPointer, byteCount: 64)
        }

        let originOffsetBuffer = originOffsetBuffers.getNextBuffer(context)
        if let bufferPointer = originOffsetBuffer?.contents().assumingMemoryBound(to: simd_float4.self) {
            bufferPointer.pointee.x = Float(self.originOffset.x - origin.x)
            bufferPointer.pointee.y = Float(self.originOffset.y - origin.y)
            bufferPointer.pointee.z = Float(self.originOffset.z - origin.z)
        }

        if samplerToUse == .magNearest {
            encoder.setFragmentSamplerState(nearestSampler, index: 0)
        } else {
            encoder.setFragmentSamplerState(sampler, index: 0)
        }

        if let texture {
            encoder.setFragmentTexture(texture, index: 0)
        }

        // --- Render Path Selection ---
        if subdivisionFactor > 0 {
            // --- GPU Tessellation Path ---
            guard let controlPointBuffer = controlPointBuffer, let tessellationFactorBuffer = tessellationFactorBuffer
            else { return }

            let pipeline = is3d ? tessellationRenderState3D : tessellationRenderState2D

            encoder.setRenderPipelineState(pipeline)
            encoder.setVertexBuffer(controlPointBuffer, offset: 0, index: 0)
            encoder.setVertexBuffer(vpMatrixBuffer, offset: 0, index: 1)

            if let mMatrixPointer = UnsafeRawPointer(bitPattern: Int(mMatrix)) {
                encoder.setVertexBytes(mMatrixPointer, length: 64, index: 2)
            }

            encoder.setVertexBuffer(originOffsetBuffer, offset: 0, index: 3)

            if is3d, let originBuffer = self.originBuffer {
                encoder.setVertexBuffer(originBuffer, offset: 0, index: 4)
            }

            encoder.setTessellationFactorBuffer(tessellationFactorBuffer, offset: 0, instanceStride: 0)

            encoder.drawPatches(
                numberOfPatchControlPoints: 4,
                patchStart: 0,
                patchCount: 1,
                patchIndexBuffer: nil,
                patchIndexBufferOffset: 0,
                instanceCount: 1,
                baseInstance: 0
            )

        } else {
            // --- Standard CPU-Generated Mesh Path ---
            guard let verticesBuffer = verticesBuffer, let indicesBuffer = indicesBuffer else { return }

            shader.setupProgram(context)
            shader.preRender(context)

            encoder.setVertexBuffer(verticesBuffer, offset: 0, index: 0)
            encoder.setVertexBuffer(vpMatrixBuffer, offset: 0, index: 1)

            if shader.usesModelMatrix() {
                if let mMatrixPointer = UnsafeRawPointer(bitPattern: Int(mMatrix)) {
                    encoder.setVertexBytes(mMatrixPointer, length: 64, index: 2)
                }
            }

            encoder.setVertexBuffer(originOffsetBuffer, offset: 0, index: 3)

            encoder.drawIndexedPrimitives(type: .triangle, indexCount: indicesCount, indexType: .uint16, indexBuffer: indicesBuffer, indexBufferOffset: 0)
        }
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
    func setMinMagFilter(_ filterType: MCTextureFilterType) {
        switch filterType {
            case .NEAREST: samplerToUse = .magNearest
            case .LINEAR: samplerToUse = .magLinear
            default: break
        }
    }

    func setSubdivisionFactor(_ factor: Int32) {
        let needsUpdate = lock.withCritical { () -> Bool in
            if self.subdivisionFactor != factor {
                self.subdivisionFactor = factor
                return true
            }
            return false
        }

        if needsUpdate {
            if factor > 0 {
                updateTessellationFactors()
            }
            // Re-run setFrame to rebuild buffers for the correct path (CPU vs GPU)
            if let frame = self.frame, let textureCoordinates = self.textureCoordinates {
                setFrame(frame, textureCoordinates: textureCoordinates, origin: self.originOffset, is3d: is3d)
            }
        }
    }

    func setFrame(
        _ frame: MCQuad3dD, textureCoordinates: MCRectD, origin: MCVec3D,
        is3d: Bool
    ) {
        lock.withCritical {
            self.is3d = is3d
            self.originOffset = origin
            self.frame = frame
            self.textureCoordinates = textureCoordinates
        }

        let sFactor = self.subdivisionFactor

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

        if sFactor > 0 {
            struct ControlPoint {
                var position: SIMD3<Float>
                var textureCoordinate: SIMD2<Float>
                var normal: SIMD3<Float>
            }

            func getNormal(_ coordinate: MCVec3D) -> SIMD3<Float> {
                if is3d {
                    let x = 1.0 * sin(coordinate.y) * cos(coordinate.x)
                    let y = 1.0 * cos(coordinate.y)
                    let z = -1.0 * sin(coordinate.y) * sin(coordinate.x)
                    return MCVec3D(x: x, y: y, z: z).simd3
                } else {
                    return SIMD3<Float>(0, 0, 1)
                }
            }

            // Bilinear interpolation for geographic coordinates
            func interpolateCoord(u: Double, v: Double) -> MCVec3D {
                let p1 = frame.topLeft.interpolate(frame.topRight, t: u)     // Top edge
                let p2 = frame.bottomLeft.interpolate(frame.bottomRight, t: u) // Bottom edge
                return p1.interpolate(p2, t: v)
            }

            // Interpolation for texture coordinates
            func getTexCoord(u: Double, v: Double) -> SIMD2<Float> {
                return SIMD2<Float>(
                    Float(textureCoordinates.x + u * textureCoordinates.width),
                    Float(textureCoordinates.y + v * textureCoordinates.height)
                )
            }

            let patchUVStarts: [(u: Double, v: Double)] = [
                (0.0, 0.0), // Top-Left sub-quad
                (0.5, 0.0), // Top-Right sub-quad
                (0.0, 0.5), // Bottom-Left sub-quad
                (0.5, 0.5), // Bottom-Right sub-quad
            ]

            var controlPoints: [ControlPoint] = []
            controlPoints.reserveCapacity(16)

            for patchUV in patchUVStarts {
                let u0 = patchUV.u, v0 = patchUV.v
                let u1 = u0 + 0.5, v1 = v0 + 0.5

                let corners_uv_ordered = [
                    (u: u0, v: v0), // Top-Left corner of sub-patch
                    (u: u1, v: v0), // Top-Right corner of sub-patch
                    (u: u0, v: v1), // Bottom-Left corner of sub-patch
                    (u: u1, v: v1)  // Bottom-Right corner of sub-patch
                ]

                for corner_uv in corners_uv_ordered {
                    let geoCoord = interpolateCoord(u: corner_uv.u, v: corner_uv.v)
                    controlPoints.append(ControlPoint(
                        position: transform(geoCoord).simd3,
                        textureCoordinate: getTexCoord(u: corner_uv.u, v: corner_uv.v),
                        normal: getNormal(geoCoord)
                    ))
                }
            }

            self.controlPointBuffer.copyOrCreate(bytes: controlPoints, length: MemoryLayout<ControlPoint>.stride * 16, device: device)

            if is3d {
                var originVec = SIMD4<Float>(x: Float(origin.x), y: Float(origin.y), z: Float(origin.z), w: 0)
                self.originBuffer?.contents().copyMemory(from: &originVec, byteCount: MemoryLayout<SIMD4<Float>>.stride)
            }

            lock.withCritical {
                self.verticesBuffer = nil
                self.indicesBuffer = nil
                self.indicesCount = 0
            }

        } else {
            // --- CPU-GENERATED MESH PATH (Unchanged) ---
            let vertices: [Vertex3DTexture] = [
                Vertex3DTexture(
                    position: transform(frame.bottomLeft),
                    textureU: textureCoordinates.xF,
                    textureV: textureCoordinates.yF + textureCoordinates.heightF
                ),
                Vertex3DTexture(
                    position: transform(frame.topLeft),
                    textureU: textureCoordinates.xF,
                    textureV: textureCoordinates.yF
                ),
                Vertex3DTexture(
                    position: transform(frame.topRight),
                    textureU: textureCoordinates.xF + textureCoordinates.widthF,
                    textureV: textureCoordinates.yF
                ),
                Vertex3DTexture(
                    position: transform(frame.bottomRight),
                    textureU: textureCoordinates.xF + textureCoordinates.widthF,
                    textureV: textureCoordinates.yF + textureCoordinates.heightF
                ),
            ]

            let indices: [UInt16] = [0, 2, 1, 0, 3, 2]

            lock.withCritical {
                self.verticesBuffer.copyOrCreate(bytes: vertices, length: MemoryLayout<Vertex3DTexture>.stride * vertices.count, device: device)
                self.indicesBuffer.copyOrCreate(bytes: indices, length: MemoryLayout<UInt16>.stride * indices.count, device: device)
                self.indicesCount = self.indicesBuffer != nil ? indices.count : 0

                self.controlPointBuffer = nil
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

    func asMaskingObject() -> MCMaskingObjectInterface? {
        self
    }
}

fileprivate extension MCVec3D {
    var simd3: SIMD3<Float> {
        SIMD3<Float>(xF, yF, zF)
    }
    func interpolate(_ other: MCVec3D, t: Double) -> MCVec3D {
        return MCVec3D(
            x: self.x + (other.x - self.x) * t,
            y: self.y + (other.y - self.y) * t,
            z: self.z + (other.z - self.z) * t
        )
    }
}

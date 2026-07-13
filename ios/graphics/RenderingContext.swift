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

public enum MetalBufferIndex {
    public static let globalVpMatrix = 20
    public static let globalOrigin = 21
    public static let globalScreenPixelAsRealMeterFactor = 22
    public static let globalTime = 23
}

@objc
public class RenderingContext: NSObject, @unchecked Sendable {
    public weak var encoder: MTLRenderCommandEncoder?
    public weak var computeEncoder: MTLComputeCommandEncoder?
    public weak var sceneView: MCMapView?

    public weak var renderTarget: RenderTargetTexture?

    public static let bufferCount = 3  // Triple buffering
    private(set) var currentBufferIndex = 0

    public private(set) var time: Float = 0

    private var globalVpMatrixBuffers: MultiBuffer<simd_float4x4> = .init(device: MetalContext.current.device)
    private var globalIdentityVpMatrixBuffers: MultiBuffer<simd_float4x4> = .init(device: MetalContext.current.device)
    private var globalOriginBuffers: MultiBuffer<simd_float4> = .init(device: MetalContext.current.device)
    private var globalScreenPixelAsRealMeterFactorBuffers: MultiBuffer<simd_float1> = .init(device: MetalContext.current.device)
    private var globalTimeBuffers: MultiBuffer<simd_float1> = .init(device: MetalContext.current.device)
    private var globalRenderVpMatrixUsesIdentity = false

    private let start = Date()

    public func beginFrame() {
        currentBufferIndex =
            (currentBufferIndex + 1) % RenderingContext.bufferCount
        time = Float(-start.timeIntervalSinceNow)
    }

    public var cullMode: MCRenderingCullMode?

    private var offscreenRenderTargetsByName: [String: any MCRenderTargetInterface] = [:]
    private var orderedOffscreenRenderTargetNames: [String] = []

    public lazy var mask: MTLDepthStencilState? = {
        let descriptor = MTLStencilDescriptor()
        descriptor.stencilCompareFunction = .always
        descriptor.stencilFailureOperation = .keep
        descriptor.depthFailureOperation = .keep
        descriptor.depthStencilPassOperation = .replace
        descriptor.writeMask = 0b1100_0000
        let depthStencilDescriptor = MTLDepthStencilDescriptor()
        depthStencilDescriptor.frontFaceStencil = descriptor
        depthStencilDescriptor.backFaceStencil = descriptor
        return MetalContext.current.device.makeDepthStencilState(
            descriptor: depthStencilDescriptor)
    }()

    public lazy var polygonMask: MTLDepthStencilState? = {
        let descriptor = MTLStencilDescriptor()
        descriptor.stencilCompareFunction = .always
        descriptor.stencilFailureOperation = .keep
        descriptor.depthFailureOperation = .keep
        descriptor.depthStencilPassOperation = .replace
        descriptor.writeMask = 0b1100_0000
        let depthStencilDescriptor = MTLDepthStencilDescriptor()
        depthStencilDescriptor.frontFaceStencil = descriptor
        depthStencilDescriptor.backFaceStencil = descriptor
        return MetalContext.current.device.makeDepthStencilState(
            descriptor: depthStencilDescriptor)
    }()

    public lazy var defaultMask: MTLDepthStencilState? = {
        let descriptor = MTLStencilDescriptor()
        descriptor.stencilCompareFunction = .always
        descriptor.depthStencilPassOperation = .keep
        let depthStencilDescriptor = MTLDepthStencilDescriptor()
        depthStencilDescriptor.frontFaceStencil = descriptor
        depthStencilDescriptor.backFaceStencil = descriptor
        return MetalContext.current.device.makeDepthStencilState(
            descriptor: depthStencilDescriptor)
    }()

    public lazy var defaultMaskDepth: MTLDepthStencilState? = {
        let descriptor = MTLStencilDescriptor()
        descriptor.stencilCompareFunction = .always
        descriptor.depthStencilPassOperation = .keep
        let depthStencilDescriptor = MTLDepthStencilDescriptor()
        depthStencilDescriptor.depthCompareFunction = .lessEqual
        depthStencilDescriptor.isDepthWriteEnabled = true
        depthStencilDescriptor.frontFaceStencil = descriptor
        depthStencilDescriptor.backFaceStencil = descriptor
        return MetalContext.current.device.makeDepthStencilState(
            descriptor: depthStencilDescriptor)
    }()

    public var aspectRatio: Float {
        viewportState.aspectRatio
    }

    private var viewportState = ViewportState()

    var isScissoringDirty = false

    var currentPipeline: MTLRenderPipelineState?
    var pendingStencilClearMask: UInt8 = 0xFF
    private var stencilWriteStates: [UInt32: MTLDepthStencilState] = [:]

    open func setRenderPipelineStateIfNeeded(
        _ pipelineState: MTLRenderPipelineState
    ) {
        guard currentPipeline !== pipelineState else {
            return
        }

        currentPipeline = pipelineState
        encoder?.setRenderPipelineState(pipelineState)
    }

    /// a Quad that fills the whole viewport
    /// this is needed to clear the stencilbuffer
    lazy var stencilClearQuad: Quad2d = {
        let quad = Quad2d(
            shader: ClearStencilShader(), metalContext: .current,
            label: "ClearStencil")
        quad.setFrame(
            .init(
                topLeft: .init(x: 1, y: -1, z: 0),
                topRight: .init(x: -1, y: -1, z: 0),
                bottomRight: .init(x: -1, y: 1, z: 0),
                bottomLeft: .init(x: 1, y: 1, z: 0)),
            textureCoordinates: .init(x: 0, y: 0, width: 0, height: 0),
            origin: .init(x: 0, y: 0, z: 0), is3d: false)
        quad.setup(self)
        return quad
    }()

    public func clearStencilBuffer(mask: UInt8 = 0xFF) {
        guard let encoder else { return }
        pendingStencilClearMask = mask
        stencilClearQuad.render(
            encoder: encoder,
            context: self,
            renderPass: .init(
                renderPass: 0,
                isPassMasked: false,
                renderTarget: renderTarget,
                stencilReadMask: 0,
                stencilReadReference: 0,
                stencilWriteMask: 0,
                stencilWriteReference: 0
            ),
            vpMatrix: 0,
            mMatrix: 0,
            origin: .init(x: 0, y: 0, z: 0),
            isMasked: false,
            screenPixelAsRealMeterFactor: 1,
            isScreenSpaceCoords: true
        )
    }
}

extension RenderingContext: MCRenderingContextInterface {
    public func getCreateOffscreenRenderTarget(_ name: String) -> (any MCRenderTargetInterface)? {
        if let renderTarget = offscreenRenderTargetsByName[name] {
            return renderTarget
        }

        let renderTarget = RenderTargetTexture(name: name)
        offscreenRenderTargetsByName[name] = renderTarget
        orderedOffscreenRenderTargetNames.append(name)
        return renderTarget
    }

    public func deleteOffscreenRenderTarget(_ name: String) {
        guard offscreenRenderTargetsByName.removeValue(forKey: name) != nil else {
            return
        }

        orderedOffscreenRenderTargetNames.removeAll { $0 == name }
    }

    public func getOffscreenRenderTargets() -> [any MCRenderTargetInterface] {
        orderedOffscreenRenderTargetNames.compactMap { offscreenRenderTargetsByName[$0] }
    }

    public func asOpenGlRenderingContext() -> (any MCOpenGlRenderingContextInterface)? {
        nil
    }

    public func setCulling(_ mode: MCRenderingCullMode) {
        self.cullMode = mode
    }

    public func preRenderStencilMask() {
        // No global stencil enable on Metal; binding a depth-stencil state activates it.
        // Must not touch stencil contents; read passes rely on bits written in earlier passes.
    }

    public func postRenderStencilMask() {
        encoder?.setDepthStencilState(defaultMask)
    }

    public func clearStencilMask(_ clearMask: Int32) {
        guard clearMask != 0 else { return }
        clearStencilBuffer(mask: UInt8(truncatingIfNeeded: clearMask))
    }

    public func setupStencilWriteMask(_ writeMask: Int32, reference: Int32) {
        let writeMask = UInt8(truncatingIfNeeded: writeMask)
        let reference = UInt8(truncatingIfNeeded: reference)
        let key = UInt32(writeMask) << 8 | UInt32(reference)
        let state: MTLDepthStencilState?
        if let cached = stencilWriteStates[key] {
            state = cached
        } else {
            let descriptor = MTLStencilDescriptor()
            descriptor.stencilCompareFunction = .always
            descriptor.stencilFailureOperation = .keep
            descriptor.depthFailureOperation = .keep
            descriptor.depthStencilPassOperation = .replace
            descriptor.writeMask = UInt32(writeMask)
            let depthStencilDescriptor = MTLDepthStencilDescriptor()
            depthStencilDescriptor.frontFaceStencil = descriptor
            depthStencilDescriptor.backFaceStencil = descriptor
            state = MetalContext.current.device.makeDepthStencilState(descriptor: depthStencilDescriptor)
            guard let state else {
                return
            }
            stencilWriteStates[key] = state
        }
        encoder?.setDepthStencilState(state)
        encoder?.setStencilReferenceValue(UInt32(reference))
    }

    public func setupDrawFrame(_ vpMatrix: Int64, origin: MCVec3D, screenPixelAsRealMeterFactor: Double) {
        currentPipeline = nil
        updateGlobalBuffers(
            vpMatrix: vpMatrix,
            origin: origin,
            screenPixelAsRealMeterFactor: screenPixelAsRealMeterFactor
        )
        updateIdentityVpMatrixBuffer()
        bindGlobalRenderBuffers()
        if let cullMode {
            /*
             Set the cullMode inverse in order to be consistent with opengl
             */
            switch cullMode {
                case .BACK:
                    encoder?.setCullMode(.front)
                case .FRONT:
                    encoder?.setCullMode(.back)
                case .NONE:
                    encoder?.setCullMode(.none)
                @unknown default:
                    assertionFailure()
            }
        }
    }

    public func setupComputeFrame(_ vpMatrix: Int64, origin: MCVec3D, screenPixelAsRealMeterFactor: Double) {
        updateGlobalBuffers(
            vpMatrix: vpMatrix,
            origin: origin,
            screenPixelAsRealMeterFactor: screenPixelAsRealMeterFactor
        )
        bindGlobalComputeBuffers()
    }

    public func bindGlobalRenderVpMatrix(isScreenSpaceCoords: Bool) {
        guard let encoder else { return }

        if isScreenSpaceCoords {
            guard !globalRenderVpMatrixUsesIdentity else { return }
            updateIdentityVpMatrixBuffer()
            let identityVpMatrixBuffer = globalIdentityVpMatrixBuffers.getNextBuffer(self)
            encoder.setVertexBuffer(identityVpMatrixBuffer, offset: 0, index: MetalBufferIndex.globalVpMatrix)
            encoder.setFragmentBuffer(identityVpMatrixBuffer, offset: 0, index: MetalBufferIndex.globalVpMatrix)
            globalRenderVpMatrixUsesIdentity = true
            return
        }

        guard globalRenderVpMatrixUsesIdentity else { return }
        let frameVpMatrixBuffer = globalVpMatrixBuffers.getNextBuffer(self)
        encoder.setVertexBuffer(frameVpMatrixBuffer, offset: 0, index: MetalBufferIndex.globalVpMatrix)
        encoder.setFragmentBuffer(frameVpMatrixBuffer, offset: 0, index: MetalBufferIndex.globalVpMatrix)
        globalRenderVpMatrixUsesIdentity = false
    }

    private func updateGlobalBuffers(
        vpMatrix: Int64,
        origin: MCVec3D,
        screenPixelAsRealMeterFactor: Double
    ) {
        let vpMatrixBuffer = globalVpMatrixBuffers.getNextBuffer(self)
        if let matrixPointer = UnsafeRawPointer(bitPattern: Int(vpMatrix)) {
            vpMatrixBuffer?.contents().copyMemory(from: matrixPointer, byteCount: MemoryLayout<simd_float4x4>.stride)
        } else if let bufferPointer = vpMatrixBuffer?.contents().assumingMemoryBound(to: simd_float4x4.self) {
            bufferPointer.pointee = matrix_identity_float4x4
        }

        if let originPointer = globalOriginBuffers.getNextBuffer(self)?.contents().assumingMemoryBound(to: simd_float4.self) {
            originPointer.pointee = simd_float4(Float(origin.x), Float(origin.y), Float(origin.z), 0.0)
        }

        var screenPixelAsRealMeterFactor = simd_float1(Float(screenPixelAsRealMeterFactor))
        globalScreenPixelAsRealMeterFactorBuffers
            .getNextBuffer(self)?
            .copyMemory(bytes: &screenPixelAsRealMeterFactor, length: MemoryLayout<simd_float1>.stride)

        var currentTime = simd_float1(time)
        globalTimeBuffers
            .getNextBuffer(self)?
            .copyMemory(bytes: &currentTime, length: MemoryLayout<simd_float1>.stride)
    }

    private func updateIdentityVpMatrixBuffer() {
        if let bufferPointer = globalIdentityVpMatrixBuffers.getNextBuffer(self)?.contents().assumingMemoryBound(to: simd_float4x4.self) {
            bufferPointer.pointee = matrix_identity_float4x4
        }
    }

    private func bindGlobalRenderBuffers() {
        guard let encoder else { return }
        globalRenderVpMatrixUsesIdentity = false
        let bufferBindings: [(MTLBuffer?, Int)] = [
            (globalVpMatrixBuffers.getNextBuffer(self), MetalBufferIndex.globalVpMatrix),
            (globalOriginBuffers.getNextBuffer(self), MetalBufferIndex.globalOrigin),
            (globalScreenPixelAsRealMeterFactorBuffers.getNextBuffer(self), MetalBufferIndex.globalScreenPixelAsRealMeterFactor),
            (globalTimeBuffers.getNextBuffer(self), MetalBufferIndex.globalTime),
        ]

        for (buffer, index) in bufferBindings {
            encoder.setVertexBuffer(buffer, offset: 0, index: index)
            encoder.setFragmentBuffer(buffer, offset: 0, index: index)
        }
    }

    private func bindGlobalComputeBuffers() {
        guard let computeEncoder else { return }
        let bufferBindings: [(MTLBuffer?, Int)] = [
            (globalVpMatrixBuffers.getNextBuffer(self), MetalBufferIndex.globalVpMatrix),
            (globalOriginBuffers.getNextBuffer(self), MetalBufferIndex.globalOrigin),
            (globalScreenPixelAsRealMeterFactorBuffers.getNextBuffer(self), MetalBufferIndex.globalScreenPixelAsRealMeterFactor),
            (globalTimeBuffers.getNextBuffer(self), MetalBufferIndex.globalTime),
        ]

        for (buffer, index) in bufferBindings {
            computeEncoder.setBuffer(buffer, offset: 0, index: index)
        }
    }

    public func onSurfaceCreated() {
    }

    public func setViewportSize(_ newSize: MCVec2I) {
        viewportState.setViewportSize(x: newSize.x, y: newSize.y)
    }

    public func getViewportSize() -> MCVec2I {
        viewportState.viewportSize
    }

    public func setBackgroundColor(_ color: MCColor) {
        Task { @MainActor in
            self.sceneView?.clearColor = color.metalColor
        }
    }

    public func applyScissorRect(_ scissorRect: MCRectI?) {
        if let sr = scissorRect {
            encoder?.setScissorRect(sr.scissorRect)
            isScissoringDirty = true
        } else if isScissoringDirty {
            var s = CGSize(width: 1.0, height: 1.0)
            if Thread.isMainThread {
                MainActor.assumeIsolated {
                    s =
                        self.sceneView?.frame.size
                        ?? CGSize(width: 1.0, height: 1.0)
                    let scale = MCDisplayMetrics.nativeScale(for: self.sceneView)
                    s.width = scale * s.width
                    s.height = scale * s.height
                }
            } else {
                DispatchQueue.main.sync {
                    s =
                        self.sceneView?.frame.size
                        ?? CGSize(width: 1.0, height: 1.0)
                    let scale = MCDisplayMetrics.nativeScale(for: self.sceneView)
                    s.width = scale * s.width
                    s.height = scale * s.height
                }
            }

            var size = getViewportSize().scissorRect
            size.width = min(size.width, Int(s.width))
            size.height = min(size.height, Int(s.height))

            encoder?.setScissorRect(size)
            isScissoringDirty = false
        }
    }
}

extension MCRectI {
    fileprivate var scissorRect: MTLScissorRect {
        MTLScissorRect(
            x: Int(x), y: Int(y), width: Int(width), height: Int(height))
    }
}

extension MCVec2I {
    fileprivate var scissorRect: MTLScissorRect {
        MTLScissorRect(x: 0, y: 0, width: Int(x), height: Int(y))
    }
}

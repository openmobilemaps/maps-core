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

open class BaseGraphicsObject: @unchecked Sendable {
    private weak var context: MCRenderingContextInterface!

    public let device: MTLDevice

    public let sampler: MTLSamplerState

    public var label: String

    var maskInverse = false
    public var ready = false
    private var readMaskStencilStates: [UInt32: MTLDepthStencilState] = [:]
    private var readMaskDepthStencilStates: [UInt32: MTLDepthStencilState] = [:]

    var isReadyFlag = false

    // this lock is locked in the rendering cycle when accessing properties of graphics object
    // therefore it has to be held for the shortest time possible
    public let lock = OSLock()

    public var originOffset: MCVec3D = .init(x: 0, y: 0, z: 0)

    public var originOffsetBuffers: MultiBuffer<simd_float4>

    public init(device: MTLDevice, sampler: MTLSamplerState, label: String = "") {
        self.device = device
        self.sampler = sampler
        self.label = label
        self.originOffsetBuffers = .init(device: device)
    }

    open func render(
        encoder _: MTLRenderCommandEncoder,
        context _: RenderingContext,
        renderPass: MCRenderPassConfig,
        vpMatrix _: Int64,
        mMatrix _: Int64,
        origin: MCVec3D,
        isMasked _: Bool,
        screenPixelAsRealMeterFactor _: Double,
        isScreenSpaceCoords _: Bool
    ) {
        fatalError("has to be overwritten by subclass")
    }

    open func compute(
        encoder _: MTLComputeCommandEncoder,
        context _: RenderingContext
    ) {
        // subclasses may override
    }
}

extension BaseGraphicsObject: MCGraphicsObjectInterface {
    public func setup(_ context: MCRenderingContextInterface?) {
        self.context = context
        self.ready = true
    }

    public func clear() {
        self.ready = false
    }

    public func pause() {
        // no-op
    }

    public func resume(_ context: MCRenderingContextInterface?) {
        // no-op
    }

    open func isReady() -> Bool { ready }

    open func setDebugLabel(_ label: String) {
        self.label += ": \(label)"
    }

    public func setIsInverseMasked(_ inversed: Bool) {
        maskInverse = inversed
    }

    public func render(
        _ context: MCRenderingContextInterface?, renderPass: MCRenderPassConfig,
        vpMatrix: Int64, mMatrix: Int64, origin: MCVec3D, isMasked: Bool,
        screenPixelAsRealMeterFactor: Double, isScreenSpaceCoords: Bool
    ) {
        guard isReady(),
            let context = context as? RenderingContext,
            let encoder = context.encoder
        else { return }

        render(
            encoder: encoder,
            context: context,
            renderPass: renderPass,
            vpMatrix: vpMatrix,
            mMatrix: mMatrix,
            origin: origin,
            isMasked: isMasked,
            screenPixelAsRealMeterFactor: screenPixelAsRealMeterFactor,
            isScreenSpaceCoords: isScreenSpaceCoords)
    }

    public func compute(
        _ context: (any MCRenderingContextInterface)?,
        renderPass: MCRenderPassConfig
    ) {
        guard let context = context as? RenderingContext,
            let encoder = context.computeEncoder
        else { return }
        compute(encoder: encoder, context: context)
    }

    // MARK: - Stencil

    public func maskStencilState(
        readMask: UInt32 = 0b1111_1111, writeMask: UInt32 = 0b0000_0000
    ) -> MTLDepthStencilState? {
        let s = MTLStencilDescriptor()
        s.stencilCompareFunction = .equal
        s.stencilFailureOperation = .zero
        s.depthFailureOperation = .keep
        s.depthStencilPassOperation = .keep
        s.readMask = readMask
        s.writeMask = writeMask

        let desc = MTLDepthStencilDescriptor()
        desc.frontFaceStencil = s
        desc.backFaceStencil = s

        return device.makeDepthStencilState(descriptor: desc)
    }

    public func maskStencilState(for renderPass: MCRenderPassConfig) -> MTLDepthStencilState? {
        let readMask = UInt32(renderPass.stencilReadMask == 0 ? 0b1100_0000 : renderPass.stencilReadMask)
        if let state = readMaskStencilStates[readMask] {
            return state
        }
        guard let state = maskStencilState(readMask: readMask) else {
            return nil
        }
        readMaskStencilStates[readMask] = state
        return state
    }

    public func maskDepthStencilState(for renderPass: MCRenderPassConfig) -> MTLDepthStencilState? {
        let readMask = UInt32(renderPass.stencilReadMask == 0 ? 0b1100_0000 : renderPass.stencilReadMask)
        if let state = readMaskDepthStencilStates[readMask] {
            return state
        }
        let s = MTLStencilDescriptor()
        s.stencilCompareFunction = .equal
        s.stencilFailureOperation = .zero
        s.depthFailureOperation = .keep
        s.depthStencilPassOperation = .keep
        s.readMask = readMask
        s.writeMask = 0b0000_0000

        let desc = MTLDepthStencilDescriptor()
        desc.depthCompareFunction = .lessEqual
        desc.isDepthWriteEnabled = true
        desc.frontFaceStencil = s
        desc.backFaceStencil = s

        guard let state = device.makeDepthStencilState(descriptor: desc) else {
            return nil
        }
        readMaskDepthStencilStates[readMask] = state
        return state
    }

    public func maskStencilReference(for renderPass: MCRenderPassConfig) -> UInt32 {
        if renderPass.stencilReadMask != 0 {
            return UInt32(renderPass.stencilReadReference)
        }
        return maskInverse ? 0b0000_0000 : 0b1100_0000
    }

    public func applyMaskWriteState(context: RenderingContext, renderPass: MCRenderPassConfig) {
        if renderPass.stencilWriteMask != 0 {
            context.setupStencilWriteMask(renderPass.stencilWriteMask, reference: renderPass.stencilWriteReference)
        } else if let mask = context.mask {
            context.encoder?.setDepthStencilState(mask)
            context.encoder?.setStencilReferenceValue(0b1100_0000)
        }
    }

    public func renderPassMaskStencilState() -> MTLDepthStencilState? {
        let s = MTLStencilDescriptor()
        s.stencilCompareFunction = .equal
        s.stencilFailureOperation = .keep
        s.depthFailureOperation = .keep
        s.depthStencilPassOperation = .incrementWrap
        s.readMask = 0b1111_1111
        s.writeMask = 0b0000_0001

        let desc = MTLDepthStencilDescriptor()
        desc.frontFaceStencil = s
        desc.backFaceStencil = s

        return device.makeDepthStencilState(descriptor: desc)
    }

    public func renderPassMaskDepthStencilState() -> MTLDepthStencilState? {
        let s = MTLStencilDescriptor()
        s.stencilCompareFunction = .equal
        s.stencilFailureOperation = .keep
        s.depthFailureOperation = .keep
        s.depthStencilPassOperation = .keep
        s.readMask = 0b1111_1111
        s.writeMask = 0b0000_0001

        let desc = MTLDepthStencilDescriptor()
        desc.depthCompareFunction = .lessEqual
        desc.isDepthWriteEnabled = true
        desc.frontFaceStencil = s
        desc.backFaceStencil = s

        return device.makeDepthStencilState(descriptor: desc)
    }
}

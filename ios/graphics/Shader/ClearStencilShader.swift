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

class ClearStencilShader: BaseShader, @unchecked Sendable {
    private var clearStates: [UInt8: MTLDepthStencilState] = [:]

    func clearState(writeMask: UInt8) -> MTLDepthStencilState? {
        if let state = clearStates[writeMask] {
            return state
        }
        let descriptor = MTLStencilDescriptor()
        descriptor.stencilCompareFunction = .always
        descriptor.stencilFailureOperation = .zero
        descriptor.depthFailureOperation = .zero
        descriptor.depthStencilPassOperation = .zero
        descriptor.writeMask = UInt32(writeMask)
        let depthStencilDescriptor = MTLDepthStencilDescriptor()
        depthStencilDescriptor.frontFaceStencil = descriptor
        depthStencilDescriptor.backFaceStencil = descriptor
        guard let state = MetalContext.current.device.makeDepthStencilState(descriptor: depthStencilDescriptor) else {
            return nil
        }
        clearStates[writeMask] = state
        return state
    }

    init() {
        super.init(shader: .clearStencilShader)
    }

    override func setupProgram(_: MCRenderingContextInterface?) {
        if pipeline == nil {
            pipeline = MetalContext.current.pipelineLibrary.value(Pipeline(type: shader, blendMode: blendMode))
        }
    }

    override open func preRender(
        encoder _: MTLRenderCommandEncoder,
        context: RenderingContext
    ) {
        guard let encoder = context.encoder,
            let pipeline
        else { return }

        context.setRenderPipelineStateIfNeeded(pipeline)
        encoder.setDepthStencilState(clearState(writeMask: context.pendingStencilClearMask))
    }
}

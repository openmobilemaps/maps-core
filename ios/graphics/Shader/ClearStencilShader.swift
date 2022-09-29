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

class ClearStencilShader: BaseShader {
    private var pipeline: MTLRenderPipelineState?

    lazy var clearMask: MTLDepthStencilState? = {
        let descriptor = MTLStencilDescriptor()
        descriptor.stencilCompareFunction = .always
        descriptor.stencilFailureOperation = .zero
        descriptor.depthFailureOperation = .zero
        descriptor.depthStencilPassOperation = .zero
        let depthStencilDescriptor = MTLDepthStencilDescriptor()
        depthStencilDescriptor.frontFaceStencil = descriptor
        depthStencilDescriptor.backFaceStencil = descriptor
        return MetalContext.current.device.makeDepthStencilState(descriptor: depthStencilDescriptor)
    }()

    override func setupProgram(_: MCRenderingContextInterface?) {
        if pipeline == nil {
            pipeline = MetalContext.current.pipelineLibrary.value(Pipeline.clearStencilShader.rawValue)
        }
    }

    override func preRender(_ context: MCRenderingContextInterface?, pass: MCRenderPassConfig) {
        guard let encoder = (context as? RenderingContext)?.encoder(pass: pass),
              let pipeline = pipeline else { return }

        encoder.setRenderPipelineState(pipeline)
        encoder.setDepthStencilState(clearMask)
    }
}

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

public class SkyboxShader: BaseShader {

    private var pipeline: MTLRenderPipelineState?

    private let shader : Pipeline
    private let buffer: MTLBuffer

    public init(shader : Pipeline = Pipeline.skyboxShader) {
        self.shader = shader
        guard let buffer = MetalContext.current.device.makeBuffer(length: MemoryLayout<Float>.stride, options: []) else { fatalError("Could not create buffer") }
        self.buffer = buffer
    }

    override public func setupProgram(_ context: MCRenderingContextInterface?) {
        if pipeline == nil {
            pipeline = MetalContext.current.pipelineLibrary(context).value(shader.rawValue)
        }
    }

    override public func preRender(encoder: MTLRenderCommandEncoder, context _: RenderingContext,
                            pass: MCRenderPassConfig) {
        guard let pipeline = pipeline else { return }

        encoder.setRenderPipelineState(pipeline)
        encoder.setFragmentBuffer(buffer, offset: 0, index: 1)
    }
}



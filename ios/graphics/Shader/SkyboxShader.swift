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

    private static let renderStartTime = Date()

    public init(shader : Pipeline = Pipeline.skyboxShader) {
        self.shader = shader
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

        var time = Float(-Self.renderStartTime.timeIntervalSinceNow)
        encoder.setFragmentBytes(&time, length: MemoryLayout<Float>.stride, index: 1)

        encoder.setCullMode(.front)
    }
}



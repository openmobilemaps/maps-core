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

class SphereColorShader: BaseShader {
    private var color = SIMD4<Float>([0.0, 0.0, 0.0, 0.0])

    private var pipeline: MTLRenderPipelineState?

    private let shader : Pipeline

    init(shader : Pipeline = Pipeline.sphereColorShader) {
        self.shader = shader
    }

    override func setupProgram(_ context: MCRenderingContextInterface?) {
        if pipeline == nil {
            pipeline = MetalContext.current.pipelineLibrary(context).value(shader.rawValue)


        }
    }

    override func preRender(encoder: MTLRenderCommandEncoder, context _: RenderingContext, pass: MCRenderPassConfig) {
        guard let pipeline = pipeline else { return }

        encoder.setRenderPipelineState(pipeline)
        encoder.setFragmentBytes(&color, length: MemoryLayout<SIMD4<Float>>.stride, index: 1)
    }
}


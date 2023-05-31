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

class AlphaShader: BaseShader {
    private var alpha: Float = 1.0
    private var alphaContent : UnsafeMutablePointer<Float>

    private let shader : PipelineType
    private let buffer: MTLBuffer

    init(shader : PipelineType = .alphaShader) {
        self.shader = shader
        guard let buffer = MetalContext.current.device.makeBuffer(length: MemoryLayout<Float>.stride, options: []) else { fatalError("Could not create buffer") }
        self.buffer = buffer
        self.alphaContent = self.buffer.contents().bindMemory(to: Float.self, capacity: 1)
        self.alphaContent[0] = alpha
    }

    override func setupProgram(_: MCRenderingContextInterface?) {
        if pipeline == nil {
            pipeline = MetalContext.current.pipelineLibrary.value(Pipeline(type: shader, blendMode: blendMode).json)
        }
    }

    override func preRender(encoder: MTLRenderCommandEncoder, context: RenderingContext) {
        guard let pipeline = pipeline else { return }

        context.setRenderPipelineStateIfNeeded(pipeline)
        encoder.setFragmentBuffer(buffer, offset: 0, index: 1)
    }
}

extension AlphaShader: MCAlphaShaderInterface {
    func updateAlpha(_ value: Float) {
        alpha = value
        self.alphaContent[0] = value
    }

    func asShaderProgram() -> MCShaderProgramInterface? {
        self
    }
}

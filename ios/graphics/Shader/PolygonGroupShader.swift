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
import UIKit

class PolygonGroupShader: BaseShader {
    private var polygonStyleBuffer: MTLBuffer

    static let styleBufferSize: Int = 32
    static let polygonStyleSize : Int = 5

    private var pipeline: MTLRenderPipelineState?

    override init() {
        guard let buffer = MetalContext.current.device.makeBuffer(length: MemoryLayout<Float>.stride * Self.styleBufferSize * Self.polygonStyleSize, options: []) else { fatalError("Could not create buffer") }
        polygonStyleBuffer = buffer
    }

    override func setupProgram(_: MCRenderingContextInterface?) {
        if pipeline == nil {
            pipeline = MetalContext.current.pipelineLibrary.value(Pipeline.polygonGroupShader.rawValue)
        }
    }

    override func preRender(encoder: MTLRenderCommandEncoder, context: RenderingContext,
                            pass: MCRenderPassConfig) {
        guard let encoder = context.encoder,
              let pipeline = pipeline else { return }

        encoder.setRenderPipelineState(pipeline)

        encoder.setFragmentBuffer(polygonStyleBuffer, offset: 0, index: 1)
    }
}

extension PolygonGroupShader: MCPolygonGroupShaderInterface {
    func setStyles(_ styles: MCSharedBytes) {
        guard styles.elementCount < Self.styleBufferSize else { fatalError("polygon style error exceeds buffer size") }

        polygonStyleBuffer.copyMemory(from: styles)
    }

    func asShaderProgram() -> MCShaderProgramInterface? {
        self
    }
}

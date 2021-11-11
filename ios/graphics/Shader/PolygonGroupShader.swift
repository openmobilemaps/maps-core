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

struct PolygonStyle: Equatable {
    var color: SIMD4<Float>
    var opacity: Float

    init(style: MCPolygonStyle) {
        color = SIMD4<Float>(style.color.simdValues)
        opacity = style.opacity
    }
}

class PolygonGroupShader: BaseShader {
    private var polygonStyleBuffer: MTLBuffer

    static let styleBufferSize: Int = 32

    private var pipeline: MTLRenderPipelineState?

    override init() {
        guard let buffer = MetalContext.current.device.makeBuffer(length: MemoryLayout<PolygonStyle>.stride * Self.styleBufferSize, options: []) else { fatalError("Could not create buffer") }
        polygonStyleBuffer = buffer
    }

    override func setupProgram(_: MCRenderingContextInterface?) {
        if pipeline == nil {
            pipeline = MetalContext.current.pipelineLibrary.value(Pipeline.polygonGroupShader.rawValue)
        }
    }

    override func preRender(encoder: MTLRenderCommandEncoder, context: RenderingContext) {
        guard let encoder = context.encoder,
              let pipeline = pipeline else { return }

        encoder.setRenderPipelineState(pipeline)

        encoder.setFragmentBuffer(polygonStyleBuffer, offset: 0, index: 1)
    }
}

extension PolygonGroupShader: MCPolygonGroupShaderInterface {
    func setStyles(_ styles: [MCPolygonStyle]) {
        guard styles.count < Self.styleBufferSize else { fatalError("line style error exceeds buffer size") }

        let mappedPolygonStyles = styles.map(PolygonStyle.init(style:))

        polygonStyleBuffer.contents().copyMemory(from: mappedPolygonStyles, byteCount: mappedPolygonStyles.count * MemoryLayout<PolygonStyle>.stride)
    }

    func asShaderProgram() -> MCShaderProgramInterface? {
        self
    }
}

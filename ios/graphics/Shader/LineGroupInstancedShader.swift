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

class LineGroupInstancedShader : BaseShader {
    private var pipeline: MTLRenderPipelineState?

    private var lineStyleBuffer: MTLBuffer
    private var lineStyleBufferContents : UnsafeMutablePointer<LineGroupStyle>

    let styleBufferSize: Int
    var screenPixelAsRealMeterFactor: Float = 1.0

    var currentStyles: [MCLineStyle] = []

    enum State {
        case normal, highlighted
    }

    private var state = State.normal

    private let shader : Pipeline

    init(styleBufferSize: Int = 256, shader : Pipeline = Pipeline.lineGroupInstancedShader) {
        self.styleBufferSize = styleBufferSize
        guard let buffer = MetalContext.current.device.makeBuffer(length: MemoryLayout<LineGroupStyle>.stride * self.styleBufferSize, options: []) else { fatalError("Could not create buffer") }
        lineStyleBuffer = buffer
        lineStyleBufferContents = buffer.contents().bindMemory(to: LineGroupStyle.self, capacity: self.styleBufferSize)
        self.shader = shader
    }

    override func setupProgram(_: MCRenderingContextInterface?) {
        if pipeline == nil {
            pipeline = MetalContext.current.pipelineLibrary.value(shader.rawValue)
        }
    }

    override func preRender(encoder: MTLRenderCommandEncoder, context _: RenderingContext) {
        guard let pipeline = pipeline else { return }

        encoder.setRenderPipelineState(pipeline)
        encoder.setVertexBytes(&screenPixelAsRealMeterFactor, length: MemoryLayout<Float>.stride, index: 3)
        encoder.setVertexBuffer(lineStyleBuffer, offset: 0, index: 4)
        encoder.setFragmentBuffer(lineStyleBuffer, offset: 0, index: 1)
    }
}

extension LineGroupInstancedShader : MCLineGroupInstancedShaderInterface {

    func setStyles(_ styles: [MCLineStyle]) {
        guard styles.count <= self.styleBufferSize else { fatalError("line style error exceeds buffer size") }

        currentStyles = styles
        for (i,l) in styles.enumerated() {
            lineStyleBufferContents[i] = LineGroupStyle(style: l, highlighted: state == .highlighted)
        }
    }

//    func setStyles(_ styles: [MCLineStyle]) {
//        guard styles.elementCount < self.styleBufferSize else { fatalError("line style error exceeds buffer size") }
//
//        lineStyleBuffer.copyMemory(from: styles)
//    }

    func asShaderProgram() -> MCShaderProgramInterface? {
        self
    }
}


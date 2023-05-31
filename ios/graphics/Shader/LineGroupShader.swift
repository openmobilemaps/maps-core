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

struct LineGroupStyle: Equatable {
    var width: Float
    var colorR: Float
    var colorG: Float
    var colorB: Float
    var colorA: Float
    var gapColorR: Float
    var gapColorG: Float
    var gapColorB: Float
    var gapColorA: Float
    var widthAsPixels: Float
    var opacity: Float
    var blur: Float
    var lineCap: Float
    var numDashValues: Float
    var dashValue0: Float = 0
    var dashValue1: Float = 0
    var dashValue2: Float = 0
    var dashValue3: Float = 0
    var offset: Float = 0
}

class LineGroupShader: BaseShader {
    private var lineStyleBuffer: MTLBuffer
    private var lineStyleBufferContents : UnsafeMutablePointer<LineGroupStyle>

    let styleBufferSize: Int

    private var pipeline: MTLRenderPipelineState?

    var screenPixelAsRealMeterFactor: Float = 1.0

    var currentStyles: [MCLineStyle] = []

    enum State {
        case normal, highlighted
    }

    private var state = State.normal

    init(styleBufferSize: Int = 256) {
        self.styleBufferSize = styleBufferSize
        guard let buffer = MetalContext.current.device.makeBuffer(length: MemoryLayout<LineGroupStyle>.stride * self.styleBufferSize, options: []) else { fatalError("Could not create buffer") }
        lineStyleBuffer = buffer
        lineStyleBufferContents = buffer.contents().bindMemory(to: LineGroupStyle.self, capacity: self.styleBufferSize)
    }

    override func setupProgram(_: MCRenderingContextInterface?) {
        if pipeline == nil {
            pipeline = MetalContext.current.pipelineLibrary.value(Pipeline.lineGroupShader.rawValue)
        }
    }

    override func preRender(encoder: MTLRenderCommandEncoder, context: RenderingContext) {
        guard let encoder = context.encoder,
              let pipeline = pipeline else { return }

        context.setRenderPipelineStateIfNeeded(pipeline)

        encoder.setVertexBytes(&screenPixelAsRealMeterFactor, length: MemoryLayout<Float>.stride, index: 2)

        encoder.setVertexBuffer(lineStyleBuffer, offset: 0, index: 4)

        encoder.setFragmentBuffer(lineStyleBuffer, offset: 0, index: 1)
    }
}

extension LineGroupShader: MCLineGroupShaderInterface {

    func setStyles(_ styles: MCSharedBytes) {
        guard styles.elementCount < self.styleBufferSize else { fatalError("line style error exceeds buffer size") }
        lineStyleBuffer.copyMemory(from: styles)
    }

    func setDashingScaleFactor(_ factor: Float) {
        
    }

    func asShaderProgram() -> MCShaderProgramInterface? {
        self
    }
}

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

    var color: SIMD4<Float>
    var gapColor: SIMD4<Float>

    var widthAsPixels: Int8

    var opacity: Float

    var blur: Float

    var lineCap: Int8

    var numDashValues: Int8

    var dashValue0: Float = 0
    var dashValue1: Float = 0
    var dashValue2: Float = 0
    var dashValue3: Float = 0

    var offset: Float = 0

    init(style: MCLineStyle, highlighted: Bool) {
        width = style.width
        if highlighted {
            color = style.color.highlighted.simdValues
            gapColor = style.gapColor.highlighted.simdValues
        } else {
            color = style.color.normal.simdValues
            gapColor = style.gapColor.normal.simdValues
        }
        widthAsPixels = style.widthType == .SCREEN_PIXEL ? 1 : 0
        
        opacity = style.opacity

        blur = style.blur

        numDashValues = Int8(style.dashArray.count)

        offset = style.offset

        // if you think, you can do this faster with a loop, you can try, but we need
        // a proof from the profiler
        dashValue0 = (numDashValues > 0 ? Float(truncating: style.dashArray[0]) : 0.0)
        dashValue1 = (numDashValues > 1 ? Float(truncating: style.dashArray[1]) : 0.0) + dashValue0
        dashValue2 = (numDashValues > 2 ? Float(truncating: style.dashArray[2]) : 0.0) + dashValue1
        dashValue3 = (numDashValues > 3 ? Float(truncating: style.dashArray[3]) : 0.0) + dashValue2

        switch style.lineCap {
            case .BUTT:
                lineCap = 0
            case .ROUND:
                lineCap = 1
            case .SQUARE:
                lineCap = 2
            @unknown default:
                assertionFailure("Line Cap type not supported")
                lineCap = 1
        }
    }
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

        encoder.setRenderPipelineState(pipeline)
        encoder.setVertexBytes(&screenPixelAsRealMeterFactor, length: MemoryLayout<Float>.stride, index: 2)
        encoder.setVertexBuffer(lineStyleBuffer, offset: 0, index: 3)
        encoder.setFragmentBuffer(lineStyleBuffer, offset: 0, index: 1)
    }
}

extension LineGroupShader: MCLineGroupShaderInterface {

    func setStyles(_ styles: [MCLineStyle]) {
        guard styles.count <= self.styleBufferSize else { fatalError("line style error exceeds buffer size") }

        currentStyles = styles
        for (i,l) in styles.enumerated() {
            lineStyleBufferContents[i] = LineGroupStyle(style: l, highlighted: state == .highlighted)
        }
    }

    func setStyles(_ styles: MCSharedBytes) {
        guard styles.elementCount < self.styleBufferSize else { fatalError("line style error exceeds buffer size") }

        lineStyleBuffer.copyMemory(from: styles)
    }

    func asShaderProgram() -> MCShaderProgramInterface? {
        self
    }
}

extension LineGroupShader: MCColorLineShaderInterface {
    func setStyle(_ lineStyle: MCLineStyle) {
        setStyles([lineStyle])
    }

    func setHighlighted(_ highlighted: Bool) {
        if highlighted {
            state = .highlighted
        } else if state == .highlighted {
            state = .normal
        }
        let styles = currentStyles
        currentStyles.removeAll()
        setStyles(styles)
    }
}

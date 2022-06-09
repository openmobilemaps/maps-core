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

struct LineStyle: Equatable {
    var width: Float

    var color: SIMD4<Float>
    var gapColor: SIMD4<Float>

    var widthAsPixels: Int8

    var opacity: Float

    var lineCap: Int8

    var numDashValues: Int8

    var dashValue0: Float = 0
    var dashValue1: Float = 0
    var dashValue2: Float = 0
    var dashValue3: Float = 0
    var dashValue4: Float = 0
    var dashValue5: Float = 0
    var dashValue6: Float = 0
    var dashValue7: Float = 0

    init(style: MCLineStyle) {
        width = style.width
        color = style.color.normal.simdValues
        gapColor = style.gapColor.normal.simdValues
        widthAsPixels = style.widthType == .SCREEN_PIXEL ? 1 : 0
        opacity = style.opacity

        numDashValues = Int8(style.dashArray.count)

        // if you think, you can do this faster with a loop, you can try, but we need
        // a proof from the profiler
        let size = style.dashArray.count
        dashValue0 = (size > 0 ? Float(truncating: style.dashArray[0]) : 0.0)
        dashValue1 = (size > 1 ? Float(truncating: style.dashArray[1]) : 0.0) + dashValue0
        dashValue2 = (size > 2 ? Float(truncating: style.dashArray[2]) : 0.0) + dashValue1
        dashValue3 = (size > 3 ? Float(truncating: style.dashArray[3]) : 0.0) + dashValue2
        dashValue4 = (size > 4 ? Float(truncating: style.dashArray[4]) : 0.0) + dashValue3
        dashValue5 = (size > 5 ? Float(truncating: style.dashArray[5]) : 0.0) + dashValue4
        dashValue6 = (size > 6 ? Float(truncating: style.dashArray[6]) : 0.0) + dashValue5
        dashValue7 = (size > 7 ? Float(truncating: style.dashArray[7]) : 0.0) + dashValue6

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

    static let styleBufferSize: Int = 32

    private var pipeline: MTLRenderPipelineState?

    var screenPixelAsRealMeterFactor: Float = 1.0

    var currentStyles: [MCLineStyle] = []

    override init() {
        guard let buffer = MetalContext.current.device.makeBuffer(length: MemoryLayout<LineStyle>.size * Self.styleBufferSize, options: []) else { fatalError("Could not create buffer") }
        lineStyleBuffer = buffer
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
    func setStyles(_ lineStyles: [MCLineStyle]) {
        guard lineStyles.count < Self.styleBufferSize else { fatalError("line style error exceeds buffer size") }

        guard lineStyles != currentStyles else {
            return
        }

        currentStyles = lineStyles

        var mappedLineStyles: [LineStyle] = []
        for l in lineStyles {
            mappedLineStyles.append(LineStyle(style: l))
        }

        lineStyleBuffer.contents().copyMemory(from: mappedLineStyles, byteCount: mappedLineStyles.count * MemoryLayout<LineStyle>.stride)
    }

    func asShaderProgram() -> MCShaderProgramInterface? {
        self
    }
}

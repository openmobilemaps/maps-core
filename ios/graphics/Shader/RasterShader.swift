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

struct RasterShaderStyle: Equatable {
    let opacity: Float
    let brightnessMin: Float
    let brightnessMax: Float
    let contrast: Float
    let saturation: Float
    let gamma: Float
    let brightnessShift: Float

    init(style: MCRasterShaderStyle) {
        self.opacity = style.opacity
        self.brightnessMin = style.brightnessMin
        self.brightnessMax = style.brightnessMax
        self.contrast = style.contrast > 0 ? (1 / (1 - style.contrast)) : (1 + style.contrast)
        self.saturation = style.saturation > 0 ? (1.0 - 1.0 / (1.001 - style.saturation)) : (-style.saturation)
        self.gamma = style.gamma
        self.brightnessShift = style.brightnessShift
    }
}

class RasterShader: BaseShader {
    private var rasterStyleBuffer: MTLBuffer
    private var rasterStyleBufferContents: UnsafeMutablePointer<RasterShaderStyle>

    private let shader: PipelineType

    init(shader: PipelineType = .rasterShader) {
        self.shader = shader
        guard let buffer = MetalContext.current.device.makeBuffer(length: MemoryLayout<RasterShaderStyle>.stride, options: []) else { fatalError("Could not create buffer") }
        self.rasterStyleBuffer = buffer
        self.rasterStyleBufferContents = self.rasterStyleBuffer.contents().bindMemory(to: RasterShaderStyle.self, capacity: 1)
        self.rasterStyleBufferContents[0] = RasterShaderStyle(style: .default())
    }

    override func setupProgram(_: MCRenderingContextInterface?) {
        if pipeline == nil {
            pipeline = MetalContext.current.pipelineLibrary.value(Pipeline(type: shader, blendMode: blendMode).json)
        }
    }

    override func preRender(encoder: MTLRenderCommandEncoder, context: RenderingContext) {
        guard let pipeline else { return }

        context.setRenderPipelineStateIfNeeded(pipeline)
        encoder.setFragmentBuffer(rasterStyleBuffer, offset: 0, index: 1)
    }
}

extension RasterShader: MCRasterShaderInterface {
    func setStyle(_ style: MCRasterShaderStyle) {
        rasterStyleBufferContents[0] = RasterShaderStyle(style: style)
    }

    func asShaderProgram() -> MCShaderProgramInterface? {
        self
    }
}

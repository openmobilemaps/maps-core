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
@preconcurrency import Metal

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

open class RasterShader: BaseShader, @unchecked Sendable {
    private var rasterStyleBuffer: MTLBuffer
    private var rasterStyleBufferContents: UnsafeMutablePointer<RasterShaderStyle>
    private var cameraPositionBuffer: MultiBuffer<SIMD4<Float>>

    override public init(shader: PipelineType = .rasterShader) {
        guard let buffer = MetalContext.current.device.makeBuffer(length: MemoryLayout<RasterShaderStyle>.stride, options: []) else { fatalError("Could not create buffer") }
        self.rasterStyleBuffer = buffer
        self.rasterStyleBufferContents = self.rasterStyleBuffer.contents().bindMemory(to: RasterShaderStyle.self, capacity: 1)
        self.cameraPositionBuffer = .init(device: MetalContext.current.device)
        self.rasterStyleBufferContents[0] = RasterShaderStyle(style: .default())
        super.init(shader: shader)
    }

    override open func setupProgram(_: MCRenderingContextInterface?) {
        if pipeline == nil {
            pipeline = MetalContext.current.pipelineLibrary.value(Pipeline(type: shader, blendMode: blendMode))
        }
    }

    override open func preRender(encoder: MTLRenderCommandEncoder, context: RenderingContext) {
        guard let pipeline else { return }

        context.setRenderPipelineStateIfNeeded(pipeline)
        encoder.setFragmentBuffer(rasterStyleBuffer, offset: 0, index: 1)

        if let buffer = cameraPositionBuffer.getNextBuffer(context) {
            let pointer = buffer.contents().assumingMemoryBound(to: SIMD4<Float>.self)
            if let cam = context.sceneView?.camera.getLastCameraPosition() {
                pointer.pointee = SIMD4<Float>(cam.xF, cam.yF, cam.zF, 1.0)
            } else {
                pointer.pointee = SIMD4<Float>(0.0, 0.0, 0.0, 1.0)
            }
            encoder.setFragmentBuffer(buffer, offset: 0, index: 2)
        }
    }
}

extension RasterShader: MCRasterShaderInterface {
    open func setStyle(_ style: MCRasterShaderStyle) {
        rasterStyleBufferContents[0] = RasterShaderStyle(style: style)
    }

    open func asShaderProgram() -> MCShaderProgramInterface? {
        self
    }
}

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

struct StretchShaderInfoSwift: Equatable {
    let scaleX: Float
    /** all stretch infos are between 0 and 1 */
    let stretchX0Begin: Float
    let stretchX0End: Float
    let stretchX1Begin: Float
    let stretchX1End: Float
    let scaleY: Float
    /** all stretch infos are between 0 and 1 */
    let stretchY0Begin: Float
    let stretchY0End: Float
    let stretchY1Begin: Float
    let stretchY1End: Float

    var uvOrig: SIMD2<Float>
    var uvSize: SIMD2<Float>

    init(
        info: MCStretchShaderInfo = MCStretchShaderInfo(
            scaleX: 1, stretchX0Begin: -1, stretchX0End: -1, stretchX1Begin: -1, stretchX1End: -1, scaleY: 1, stretchY0Begin: -1, stretchY0End: -1, stretchY1Begin: -1, stretchY1End: -1, uv: MCRectD(x: 0, y: 0, width: 0, height: 0))
    ) {
        scaleX = info.scaleX
        stretchX0Begin = info.stretchX0Begin
        stretchX0End = info.stretchX0End
        stretchX1Begin = info.stretchX1Begin
        stretchX1End = info.stretchX1End
        scaleY = info.scaleY
        stretchY0Begin = info.stretchY0Begin
        stretchY0End = info.stretchY0End
        stretchY1Begin = info.stretchY1Begin
        stretchY1End = info.stretchY1End

        uvOrig = SIMD2<Float>(info.uv.xF, info.uv.yF)
        uvSize = SIMD2<Float>(info.uv.widthF, info.uv.heightF)
    }
}

class StretchShader: BaseShader, @unchecked Sendable {
    private let alphaBuffer: MTLBuffer
    private var alphaContent: UnsafeMutablePointer<Float>

    private var infoContent: UnsafeMutablePointer<StretchShaderInfoSwift>
    private let infoBuffer: MTLBuffer

    private var stretchInfo = StretchShaderInfoSwift()

    override init(shader: PipelineType = .stretchShader) {
        guard let infoBuffer = MetalContext.current.device.makeBuffer(length: MemoryLayout<StretchShaderInfoSwift>.stride, options: []) else { fatalError("Could not create buffer") }
        self.infoBuffer = infoBuffer
        self.infoContent = self.infoBuffer.contents().bindMemory(to: StretchShaderInfoSwift.self, capacity: 1)
        self.infoContent[0] = stretchInfo

        guard let alphaBuffer = MetalContext.current.device.makeBuffer(length: MemoryLayout<Float>.stride, options: []) else { fatalError("Could not create buffer") }
        self.alphaBuffer = alphaBuffer
        self.alphaContent = self.alphaBuffer.contents().bindMemory(to: Float.self, capacity: 1)
        self.alphaContent[0] = 1.0

        super.init(shader: shader)
    }

    override func setupProgram(_: MCRenderingContextInterface?) {
        if pipeline == nil {
            pipeline = MetalContext.current.pipelineLibrary.value(Pipeline(type: shader, blendMode: blendMode))
        }
    }

    override func preRender(encoder: MTLRenderCommandEncoder, context: RenderingContext) {
        guard let pipeline else { return }

        context.setRenderPipelineStateIfNeeded(pipeline)
        encoder.setFragmentBuffer(alphaBuffer, offset: 0, index: 1)
        encoder.setFragmentBuffer(infoBuffer, offset: 0, index: 2)
    }
}

extension StretchShader: MCStretchShaderInterface {
    func updateStretch(_ info: MCStretchShaderInfo) {
        self.infoContent[0] = .init(info: info)
    }

    func updateAlpha(_ value: Float) {
        self.alphaContent[0] = value
    }

    func asShaderProgram() -> MCShaderProgramInterface? {
        self
    }
}

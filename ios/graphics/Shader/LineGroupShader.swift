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

class LineGroupShader: BaseShader, @unchecked Sendable {
    var lineStyleBuffer: MTLBuffer?

    var screenPixelAsRealMeterFactor: Float = 1.0

    var dashingScaleFactor: Float = 1.0


    override init(shader: PipelineType = .lineGroupShader) {
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

        encoder.setVertexBytes(&screenPixelAsRealMeterFactor, length: MemoryLayout<Float>.stride, index: 2)

        encoder.setVertexBytes(&dashingScaleFactor, length: MemoryLayout<Float>.stride, index: 3)

        encoder.setVertexBuffer(lineStyleBuffer, offset: 0, index: 4)

        encoder.setFragmentBuffer(lineStyleBuffer, offset: 0, index: 1)
    }
}

extension LineGroupShader: MCLineGroupShaderInterface {
    func setStyles(_ styles: MCSharedBytes) {
        lineStyleBuffer.copyOrCreate(from: styles, device: MetalContext.current.device)
    }

    func setDashingScaleFactor(_ factor: Float) {
        dashingScaleFactor = factor
    }

    func asShaderProgram() -> MCShaderProgramInterface? {
        self
    }
}

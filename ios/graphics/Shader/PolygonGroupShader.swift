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
    var polygonStyleBuffer: MTLBuffer?

    override init() {}

    override func setupProgram(_: MCRenderingContextInterface?) {
        if pipeline == nil {
            pipeline = MetalContext.current.pipelineLibrary.value(Pipeline(type: .polygonGroupShader, blendMode: blendMode).json)
        }
    }

    override func preRender(encoder: MTLRenderCommandEncoder, context: RenderingContext) {
        guard let encoder = context.encoder,
              let pipeline = pipeline else { return }

        context.setRenderPipelineStateIfNeeded(pipeline)

        context.setFragmentBuffer(polygonStyleBuffer, offset: 0, index: 1)
    }
}

extension PolygonGroupShader: MCPolygonGroupShaderInterface {
    func setStyles(_ styles: MCSharedBytes) {
        polygonStyleBuffer.copyOrCreate(from: styles, device: MetalContext.current.device)
    }

    func asShaderProgram() -> MCShaderProgramInterface? {
        self
    }
}

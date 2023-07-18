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

class TextureInterpolationShader: BaseShader {
    private var interpolation: Float = 0.5
    
    override func setupProgram(_: MCRenderingContextInterface?) {
        if pipeline == nil {
            pipeline = MetalContext.current.pipelineLibrary.value(Pipeline(type: .textureInterpolationShader, blendMode: .MULTIPLY).json)
        }
    }

    override func preRender(encoder: MTLRenderCommandEncoder, context: RenderingContext) {
        guard let pipeline else { return }

        context.setRenderPipelineStateIfNeeded(pipeline)
        encoder.setFragmentBytes(&interpolation, length: MemoryLayout<Double>.stride, index: 3)
    }
}

extension TextureInterpolationShader: MCTextureInterpolationShaderInterface {
    
    func setInterpolationFactor(_ interpolation: Float) {
        self.interpolation = interpolation
    }

    func asShaderProgram() -> MCShaderProgramInterface? {
        self
    }
}

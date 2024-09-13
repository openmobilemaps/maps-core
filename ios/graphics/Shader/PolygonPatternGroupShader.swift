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

class PolygonPatternGroupShader: BaseShader {
    // MARK: - Variables

    let fadeInPattern: Bool

    // MARK: - Init

    init(fadeInPattern: Bool) {
        self.fadeInPattern = fadeInPattern
        super.init()
    }

    // MARK: - Setup

    override func setupProgram(_: MCRenderingContextInterface?) {
        guard pipeline == nil else { return }

        let t: PipelineType = fadeInPattern ? .polygonPatternFadeInGroupShader : .polygonPatternGroupShader
        let pl = Pipeline(type: t, blendMode: blendMode)
        pipeline = MetalContext.current.pipelineLibrary.value(pl.json)
    }

    override func preRender(encoder: MTLRenderCommandEncoder, context: RenderingContext) {
        guard let pipeline else { return }
        context.setRenderPipelineStateIfNeeded(pipeline)
    }
}

extension PolygonPatternGroupShader: MCPolygonPatternGroupShaderInterface {
    func asShaderProgram() -> MCShaderProgramInterface? {
        self
    }
}

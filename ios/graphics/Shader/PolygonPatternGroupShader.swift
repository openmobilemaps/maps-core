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
import UIKit

class PolygonPatternGroupShader: BaseShader, @unchecked Sendable {
    // MARK: - Variables

    let fadeInPattern: Bool
    let isUnitSphere: Bool

    // MARK: - Init

    init(fadeInPattern: Bool, isUnitSphere: Bool = false) {
        self.fadeInPattern = fadeInPattern
        self.isUnitSphere = isUnitSphere
        super.init(shader: fadeInPattern ? .polygonPatternFadeInGroupShader : .polygonPatternGroupShader)
    }

    // MARK: - Setup

    override func setupProgram(_: MCRenderingContextInterface?) {
        guard pipeline == nil else { return }

        let pl = Pipeline(type: shader, blendMode: blendMode)
        pipeline = MetalContext.current.pipelineLibrary.value(pl)
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

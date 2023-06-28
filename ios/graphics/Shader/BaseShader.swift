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

open class BaseShader: MCShaderProgramInterface {
    var blendMode: MCBlendMode = .NORMAL

    var pipeline: MTLRenderPipelineState?

    public init() {
    }

    open func getProgramName() -> String {
        ""
    }

    open func setupProgram(_: MCRenderingContextInterface?) {
    }

    open func preRender(_ context: MCRenderingContextInterface?) {
        guard let context = context as? RenderingContext,
              let encoder = context.encoder else { return }
        preRender(encoder: encoder, context: context)
    }

    open func preRender(encoder _: MTLRenderCommandEncoder,
                        context _: RenderingContext) {
    }

    open func setBlendMode(_ blendMode: MCBlendMode) {
        guard blendMode != self.blendMode else { return }
        self.blendMode = blendMode
        pipeline = nil
        setupProgram(nil)
    }
}

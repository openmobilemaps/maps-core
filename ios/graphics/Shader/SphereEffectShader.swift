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

class SphereEffectShader: BaseShader, @unchecked Sendable {

    private var ellipse: [Float] = []

    private var ellipseBuffer: (any MTLBuffer)?

    private var stencilState: MTLDepthStencilState?

    override func setupProgram(_: MCRenderingContextInterface?) {
        if pipeline == nil {
            pipeline = MetalContext.current.pipelineLibrary.value(Pipeline(type: .sphereEffectShader, blendMode: blendMode).json)
        }
    }

    override func preRender(encoder: MTLRenderCommandEncoder, context: RenderingContext) {
        guard let pipeline else { return }
        context.setRenderPipelineStateIfNeeded(pipeline)
        if let ellipseBuffer {
            encoder.setFragmentBuffer(ellipseBuffer, offset: 0, index: 0)
        }
    }
}

extension SphereEffectShader: MCSphereEffectShaderInterface {
    func asShaderProgram() -> (any MCShaderProgramInterface)? {
        return self
    }

    func setEllipse(_ coefficients: [NSNumber]) {

        ellipse = coefficients.map { $0.floatValue }

        ellipseBuffer = MetalContext.current.device.makeBuffer(bytes: &ellipse, length: MemoryLayout<Float>.size * ellipse.count, options: [])
    }
}

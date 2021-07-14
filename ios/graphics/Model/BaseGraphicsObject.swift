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

class BaseGraphicsObject {
    private var context: MCRenderingContextInterface!

    let device: MTLDevice

    let sampler: MTLSamplerState

    init(device: MTLDevice, sampler: MTLSamplerState) {
        self.device = device
        self.sampler = sampler
    }

    func render(encoder _: MTLRenderCommandEncoder,
                context _: RenderingContext,
                renderPass _: MCRenderPassConfig,
                mvpMatrix _: Int64,
                screenPixelAsRealMeterFactor _: Double)
    {
        fatalError("has to be overwritten by subclass")
    }
}

extension BaseGraphicsObject: MCGraphicsObjectInterface {
    func setup(_ context: MCRenderingContextInterface?) {
        self.context = context
    }

    func clear() {}

    func isReady() -> Bool { true }

    func render(_ context: MCRenderingContextInterface?, renderPass: MCRenderPassConfig, mvpMatrix: Int64, screenPixelAsRealMeterFactor: Double) {
        guard let context = context as? RenderingContext,
              let encoder = context.encoder
        else { return }
        render(encoder: encoder,
               context: context,
               renderPass: renderPass,
               mvpMatrix: mvpMatrix,
               screenPixelAsRealMeterFactor: screenPixelAsRealMeterFactor)
    }
}

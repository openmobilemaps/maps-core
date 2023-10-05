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

open class BaseGraphicsObject {
    private weak var context: MCRenderingContextInterface!

    public let device: MTLDevice

    public let sampler: MTLSamplerState

    public var label: String

    var maskInverse = false
    public var ready = false

    var isReadyFlag = false

    // this lock is locked in the rendering cycle when accessing properties of graphics object
    // therefore it has to be held for the shortest time possible
    public let lock = OSLock()

    public init(device: MTLDevice, sampler: MTLSamplerState, label: String = "") {
        self.device = device
        self.sampler = sampler
        self.label = label
    }

    open func render(encoder _: MTLRenderCommandEncoder,
                     context _: RenderingContext,
                     renderPass _: MCRenderPassConfig,
                     mvpMatrix _: Int64,
                     isMasked _: Bool,
                     screenPixelAsRealMeterFactor _: Double) {
        fatalError("has to be overwritten by subclass")
    }
}

extension BaseGraphicsObject: MCGraphicsObjectInterface {
    public func setup(_ context: MCRenderingContextInterface?) {
        self.context = context
        self.ready = true
    }

    public func clear() {
        self.ready = false
    }

    open func isReady() -> Bool { ready }

    open func setDebugLabel(_ label: String) {
        self.label += ": \(label)"
    }

    public func setIsInverseMasked(_ inversed: Bool) {
        maskInverse = inversed
    }

    public func render(_ context: MCRenderingContextInterface?, renderPass: MCRenderPassConfig, mvpMatrix: Int64, isMasked: Bool, screenPixelAsRealMeterFactor: Double) {
        guard isReady(),
              let context = context as? RenderingContext,
              let encoder = context.encoder
        else { return }
        render(encoder: encoder,
               context: context,
               renderPass: renderPass,
               mvpMatrix: mvpMatrix,
               isMasked: isMasked,
               screenPixelAsRealMeterFactor: screenPixelAsRealMeterFactor)
    }
}

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
import MetalKit

enum SamplerKey: CaseIterable {
    case magLinear
    case magNearest

    fileprivate func create() -> MTLSamplerDescriptor {
        let samplerDescriptor = MTLSamplerDescriptor()
        samplerDescriptor.minFilter = .nearest
        samplerDescriptor.sAddressMode = .clampToEdge
        samplerDescriptor.tAddressMode = .clampToEdge

        switch self {
        case .magLinear:
            samplerDescriptor.label = "Smapler Mag Linear"
            samplerDescriptor.magFilter = .linear
        case .magNearest:
            samplerDescriptor.label = "Smapler Mag Nearest"
            samplerDescriptor.magFilter = .nearest
        }
        return samplerDescriptor
    }
}

class SamplerLibrary: StaticMetalLibrary<SamplerKey, MTLSamplerState> {
    init(device: MTLDevice) {
        super.init { (key) -> MTLSamplerState in
            let samplerDescriptor = key.create()
            guard let samplerState = device.makeSamplerState(descriptor: samplerDescriptor) else {
                fatalError("Cannot create Sampler \(key)")
            }
            return samplerState
        }
    }
}

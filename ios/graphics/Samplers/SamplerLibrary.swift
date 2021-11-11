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

public enum SamplerFactory {
    public static func descriptor(label: String, magFilter: MTLSamplerMinMagFilter) -> MTLSamplerDescriptor {
        let samplerDescriptor = MTLSamplerDescriptor()
        samplerDescriptor.minFilter = .nearest
        samplerDescriptor.sAddressMode = .clampToEdge
        samplerDescriptor.tAddressMode = .clampToEdge
        samplerDescriptor.label = label
        samplerDescriptor.magFilter = magFilter
        return samplerDescriptor
    }
}

extension SamplerFactory {
    public static func descriptor(sampler: Sampler) -> MTLSamplerDescriptor {
        return descriptor(label: sampler.label, magFilter: sampler.magFilter)
    }
}

public enum Sampler: String, CaseIterable {
    case magLinear = "magLinear"
    case magNearest = "magNearest"

    fileprivate var label: String {
        rawValue
    }

    fileprivate var magFilter: MTLSamplerMinMagFilter {
        switch self {
        case .magLinear:
            return .linear
        case .magNearest:
            return .nearest

        }
    }
}

public class SamplerLibrary: StaticMetalLibrary<String, MTLSamplerState> {
    init(device: MTLDevice) throws {
        try super.init(Sampler.allCases.map(\.rawValue)) { (key) -> MTLSamplerState in
            guard let sampler = Sampler(rawValue: key) else {
                throw LibraryError.invalidKey
            }
            let samplerDescriptor = SamplerFactory.descriptor(sampler: sampler)

            guard let samplerState = device.makeSamplerState(descriptor: samplerDescriptor) else {
                fatalError("Cannot create Sampler \(key)")
            }
            return samplerState
        }
    }
}

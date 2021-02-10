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

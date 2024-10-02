//
//  Layer.swift
//
//
//  Created by Nicolas Märki on 14.02.2024.
//

import Foundation
import MapCoreSharedModule

public protocol Layer: Sendable {
    var interface: MCLayerInterface? { get }
    var layerIndex: Int? { get }
    func forceReload()
}

public extension Layer {
    var layerIndex: Int? { nil }

    func forceReload() {
        interface?.forceReload()
    }
}

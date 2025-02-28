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
    var beforeAdding: ((MCLayerInterface, MCMapView) -> Void)? { get }
    func forceReload()
}

public extension Layer {
    var layerIndex: Int? { nil }

    func forceReload() {
        interface?.forceReload()
    }

    var beforeAdding: ((MCLayerInterface, MCMapView) -> Void)? {
        nil
    }
}

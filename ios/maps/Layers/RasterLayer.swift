//
//  RasterLayer.swift
//
//
//  Created by Nicolas Märki on 14.02.2024.
//

import Foundation
import MapCoreSharedModule

@available(iOS 13.0, *)
open class TiledRasterLayer: ObservableObject {
    public init(config: MCTiled2dMapLayerConfig, loaders: [MCLoaderInterface] = [MCTextureLoader()]) {
        self.tiledLayerInterface = MCTiled2dMapRasterLayerInterface.create(config, loaders: loaders) !! fatalError("create is non-null")
    }

    /// Create a default layer using a web mercator layer config
    /// - Parameters:
    ///   - layerName: Identifier for layer
    ///   - urlFormat: URL for tile with placeholders, e.g. https://www.sample.org/{z}/{x}/{y}.png
    public convenience init(_ layerName: String = UUID().uuidString, webMercatorUrlFormat: String) {
        self.init(config: MCDefaultTiled2dMapLayerConfigs.webMercator(layerName, urlFormat: webMercatorUrlFormat) !! fatalError("default configs are non-null"))
    }

    init(_ tiledLayerInterface: MCTiled2dMapRasterLayerInterface) {
        self.tiledLayerInterface = tiledLayerInterface
    }

    /// Shared implementation with advanced APIs
    public let tiledLayerInterface: MCTiled2dMapRasterLayerInterface
}

@available(iOS 13.0, *)
extension TiledRasterLayer: Layer {
    public var interface: MCLayerInterface? { tiledLayerInterface.asLayerInterface() !! fatalError("asLayerInterface is non-null") }
}

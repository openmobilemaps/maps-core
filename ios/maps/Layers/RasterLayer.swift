//
//  RasterLayer.swift
//
//
//  Created by Nicolas Märki on 14.02.2024.
//

import Foundation
import MapCoreSharedModule

open class TiledRasterLayer: Layer, ObservableObject, @unchecked Sendable {
    public init(
        config: MCTiled2dMapLayerConfig, loaders: [MCLoaderInterface] = [MCTextureLoader()], shader: MCShaderProgramInterface? = nil, callbackHandler: MCTiled2dMapRasterLayerCallbackInterface? = nil, layerIndex: Int? = nil, beforeAdding: ((MCTiled2dMapRasterLayerInterface, MCMapView) -> Void)? = nil
    ) {
        if let shader {
            self.tiledLayerInterface = MCTiled2dMapRasterLayerInterface.create(withShader: config, loaders: loaders, shader: shader) !! fatalError("create is non-null")
        }
        else {
            self.tiledLayerInterface = MCTiled2dMapRasterLayerInterface.create(config, loaders: loaders) !! fatalError("create is non-null")
        }
        self.tiledLayerInterface.setCallbackHandler(callbackHandler)
        self.layerIndex = layerIndex
        if let beforeAdding {
            self.beforeAdding = {
                [weak self] in
                guard let self else { return }
                beforeAdding(self.tiledLayerInterface, $1)
            }
        }
    }

    /// Create a default layer using a web mercator layer config
    /// - Parameters:
    ///   - layerName: Identifier for layer
    ///   - urlFormat: URL for tile with placeholders, e.g. https://www.sample.org/{z}/{x}/{y}.png
    public convenience init(_ layerName: String = UUID().uuidString, webMercatorUrlFormat: String, minZoomLevel: Int = 0, maxZoomLevel: Int = 20, layerIndex: Int? = nil, beforeAdding: ((MCTiled2dMapRasterLayerInterface, MCMapView) -> Void)? = nil) {
        self.init(
            config:
                MCDefaultTiled2dMapLayerConfigs
                .webMercatorCustom(layerName, urlFormat: webMercatorUrlFormat, zoomInfo: nil, minZoomLevel: Int32(minZoomLevel), maxZoomLevel: Int32(maxZoomLevel))
                !! fatalError(
                    "default configs are non-null"
                ),
            layerIndex: layerIndex,
            beforeAdding: beforeAdding
        )
    }

    init(_ tiledLayerInterface: MCTiled2dMapRasterLayerInterface, beforeAdding: ((MCLayerInterface, MCMapView) -> Void)? = nil) {
        self.tiledLayerInterface = tiledLayerInterface
        self.beforeAdding = beforeAdding
    }

    /// Shared implementation with advanced APIs
    public let tiledLayerInterface: MCTiled2dMapRasterLayerInterface

    public var layerIndex: Int?

    public var interface: MCLayerInterface? { tiledLayerInterface.asLayerInterface() !! fatalError("asLayerInterface is non-null") }

    public var beforeAdding: ((MCLayerInterface, MCMapView) -> Void)?
}

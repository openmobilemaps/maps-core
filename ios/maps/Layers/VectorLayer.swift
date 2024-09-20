//
//  VectorLayer.swift
//
//
//  Created by Nicolas Märki on 14.02.2024.
//

import Foundation
import MapCoreSharedModule

@available(iOS 13.0, *)
open class VectorLayer: Layer, ObservableObject, @unchecked Sendable {

    public init(_ layerName: String = UUID().uuidString,
                styleURL: String,
                layerIndex: Int? = nil,
                localDataProvider: MCTiled2dMapVectorLayerLocalDataProviderInterface? = nil,
                customZoomInfo: MCTiled2dMapZoomInfo? = nil,
                loaders: [MCLoaderInterface] = [MCTextureLoader()],
                fontLoader: MCFontLoader = MCFontLoader(bundle: .main)) {
        self.layerInterface = MCTiled2dMapVectorLayerInterface.createExplicitly(layerName, styleJson: styleURL, localStyleJson: nil, loaders: loaders, fontLoader: fontLoader, localDataProvider: localDataProvider, customZoomInfo: customZoomInfo, symbolDelegate: nil, sourceUrlParams: nil)
        self.layerIndex = layerIndex
    }

    public let layerInterface: MCTiled2dMapVectorLayerInterface?

    public var interface: MCLayerInterface? { layerInterface?.asLayerInterface() }

    public var layerIndex: Int?
}

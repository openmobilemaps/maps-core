//
//  VectorLayer.swift
//
//
//  Created by Nicolas Märki on 14.02.2024.
//

import Foundation
import MapCoreSharedModule

@available(iOS 13.0, *)
open class VectorLayer: Layer, ObservableObject {
    public init(_ layerName: String = UUID().uuidString, styleURL: String) {
        self.layerInterface = MCTiled2dMapVectorLayerInterface.createExplicitly(layerName, styleJson: styleURL, localStyleJson: nil, loaders: [MCTextureLoader()], fontLoader: MCFontLoader(bundle: .main), localDataProvider: nil, customZoomInfo: nil, symbolDelegate: nil, sourceUrlParams: nil)
    }

    public let layerInterface: MCTiled2dMapVectorLayerInterface?

    public var interface: MCLayerInterface? { layerInterface?.asLayerInterface() }
}

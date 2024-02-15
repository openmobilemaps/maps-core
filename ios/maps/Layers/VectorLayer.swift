//
//  File.swift
//  
//
//  Created by Nicolas Märki on 14.02.2024.
//

import Foundation
import MapCoreSharedModule

@available(iOS 13.0, *)
open class VectorLayer: Layer, ObservableObject {

    public init(_ layerName: String, styleURL: String) throws {
        guard let layer = MCTiled2dMapVectorLayerInterface.createExplicitly(layerName, styleJson: styleURL, localStyleJson: nil, loaders: [MCTextureLoader()], fontLoader: MCFontLoader(bundle: .main), localDataProvider: nil, customZoomInfo: nil, symbolDelegate: nil, sourceUrlParams: nil) else {
            throw VectorLayerError.createFailed
        }
        self.layerInterface = layer
    }


    public let layerInterface: MCTiled2dMapVectorLayerInterface

    enum VectorLayerError: Error {
        case createFailed
    }

    public var interface: MCLayerInterface? { layerInterface.asLayerInterface() !! fatalError("asLayerInterface is non-null") }
}



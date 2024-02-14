//
//  File.swift
//  
//
//  Created by Nicolas Märki on 14.02.2024.
//

import Foundation
import MapCoreSharedModule

open class VectorLayer {

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
}

extension VectorLayer: Layer {
    public var interface: MCLayerInterface { layerInterface.asLayerInterface() !! fatalError("asLayerInterface is non-null") }
}

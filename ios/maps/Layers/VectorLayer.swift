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

    public init(_ layerName: String = UUID().uuidString,
                styleURL: String,
                layerIndex: Int? = nil,
                localDataProvider: MCTiled2dMapVectorLayerLocalDataProviderInterface? = nil,
                customZoomInfo: MCTiled2dMapZoomInfo? = nil,
                loaders: [MCLoaderInterface] = [MCTextureLoader()]) {
        self.layerInterface = MCTiled2dMapVectorLayerInterface.createExplicitly(layerName, styleJson: styleURL, localStyleJson: nil, loaders: loaders, fontLoader: MCFontLoader(bundle: .main), localDataProvider: localDataProvider, customZoomInfo: customZoomInfo, symbolDelegate: nil, sourceUrlParams: nil)
        self.layerInterface?.setSelectionDelegate(selectionHandler)
        self.layerIndex = layerIndex
    }

    public let layerInterface: MCTiled2dMapVectorLayerInterface?

    public var interface: MCLayerInterface? { layerInterface?.asLayerInterface() }

    public var layerIndex: Int?

    public let selectionHandler = MCTiled2dMapVectorLayerSelectionCallbackHandler()
}

open class MCTiled2dMapVectorLayerSelectionCallbackHandler: MCTiled2dMapVectorLayerSelectionCallbackInterface {
    public var didClickBackgroundConfirmedCallback: ((_ coord: MCCoord)->Bool)?
    public var didSelectFeatureCallback: ((_ featureInfo: MCVectorLayerFeatureInfo, _ layerIdentifier: String, _ coord: MCCoord)->Bool)?
    public var didMultiSelectLayerFeaturesCallback: ((_ featureInfos: [MCVectorLayerFeatureInfo], _ layerIdentifier: String, _ coord: MCCoord)->Bool)?

    public func didClickBackgroundConfirmed(_ coord: MCCoord) -> Bool {
        didClickBackgroundConfirmedCallback?(coord) ?? false
    }

    public func didSelectFeature(_ featureInfo: MCVectorLayerFeatureInfo, layerIdentifier: String, coord: MCCoord) -> Bool {
        return didSelectFeatureCallback?(featureInfo, layerIdentifier, coord) ?? false
    }

    public func didMultiSelectLayerFeatures(_ featureInfos: [MCVectorLayerFeatureInfo], layerIdentifier: String, coord: MCCoord) -> Bool {
        didMultiSelectLayerFeaturesCallback?(featureInfos, layerIdentifier, coord) ?? false
    }
}

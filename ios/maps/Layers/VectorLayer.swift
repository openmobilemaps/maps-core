//
//  VectorLayer.swift
//
//
//  Created by Nicolas Märki on 14.02.2024.
//

import Foundation
import MapCoreSharedModule

open class VectorLayer: Layer, ObservableObject, @unchecked Sendable {
    public init(
        _ layerName: String = UUID().uuidString,
        styleURL: String,
        layerIndex: Int? = nil,
        localDataProvider: MCTiled2dMapVectorLayerLocalDataProviderInterface? = nil,
        customZoomInfo: MCTiled2dMapZoomInfo? = nil,
        loaders: [MCLoaderInterface] = [MCTextureLoader()],
        fontLoader: MCFontLoader = MCFontLoader(bundle: .main),
        beforeAdding: ((MCTiled2dMapVectorLayerInterface, MCMapView) -> Void)? = nil
    ) {
        self.layerInterface = MCTiled2dMapVectorLayerInterface.createExplicitly(
            layerName, styleJson: styleURL, localStyleJson: nil, loaders: loaders, fontLoader: fontLoader, localDataProvider: localDataProvider, customZoomInfo: customZoomInfo, symbolDelegate: nil, sourceUrlParams: nil)
        self.layerInterface?.setSelectionDelegate(selectionHandler)
        self.layerIndex = layerIndex
        if let beforeAdding {
            self.beforeAdding = {
                [weak self] in
                guard let self, let layerInterface = self.layerInterface else {
                    return
                }
                beforeAdding(layerInterface, $1)
            }
        }
    }

    public let layerInterface: MCTiled2dMapVectorLayerInterface?

    public var interface: MCLayerInterface? { layerInterface?.asLayerInterface() }

    public var layerIndex: Int?

    public let selectionHandler = MCTiled2dMapVectorLayerSelectionCallbackHandler()

    public var beforeAdding: ((MCLayerInterface, MCMapView) -> Void)?
}

open class MCTiled2dMapVectorLayerSelectionCallbackHandler: MCTiled2dMapVectorLayerSelectionCallbackInterface {
    public var didClickBackgroundConfirmedCallback: ((_ coord: MCCoord) -> Bool)?
    public var didSelectFeatureCallback: ((_ featureInfo: MCVectorLayerFeatureInfo, _ layerIdentifier: String, _ coord: MCCoord) -> Bool)?
    public var didMultiSelectLayerFeaturesCallback: ((_ featureInfos: [MCVectorLayerFeatureInfo], _ layerIdentifier: String, _ coord: MCCoord) -> Bool)?

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

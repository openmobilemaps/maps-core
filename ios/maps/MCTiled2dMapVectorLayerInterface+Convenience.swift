//
//  MCTiled2dMapVectorLayerInterface+Convenience.swift
//
//
//  Created by OpenAI Codex on 05.05.2026.
//

import Foundation

public extension MCTiled2dMapVectorLayerInterface {
    static func make(
        _ layerName: String = UUID().uuidString,
        styleURL: String,
        localDataProvider: MCTiled2dMapVectorLayerLocalDataProviderInterface? = nil,
        customZoomInfo: MCTiled2dMapZoomInfo? = nil,
        loaders: [MCLoaderInterface] = [MCTextureLoader()],
        fontLoader: MCFontLoaderInterface? = MCFontLoader(bundle: .main),
        symbolDelegate: MCTiled2dMapVectorLayerSymbolDelegateInterface? = nil,
        sourceUrlParams: [String: String]? = nil,
        selectionDelegate: MCTiled2dMapVectorLayerSelectionCallbackInterface? = nil
    ) throws -> MCTiled2dMapVectorLayerInterface {
        guard
            let layer = createExplicitly(
                layerName,
                styleJson: styleURL,
                localStyleJson: nil,
                loaders: loaders,
                fontLoader: fontLoader,
                localDataProvider: localDataProvider,
                customZoomInfo: customZoomInfo,
                symbolDelegate: symbolDelegate,
                sourceUrlParams: sourceUrlParams
            )
        else {
            throw Errors.createFailed
        }

        layer.setSelectionDelegate(selectionDelegate)
        return layer
    }

    private enum Errors: Error {
        case createFailed
    }
}

//
//  MCTiled2dMapRasterLayerInterface+Convenience.swift
//
//
//  Created by OpenAI Codex on 05.05.2026.
//

import Foundation

public extension MCTiled2dMapRasterLayerInterface {
    static func make(
        config: MCTiled2dMapLayerConfig,
        loaders: [MCLoaderInterface] = [MCTextureLoader()],
        shader: MCShaderProgramInterface? = nil,
        callbackHandler: MCTiled2dMapRasterLayerCallbackInterface? = nil
    ) throws -> MCTiled2dMapRasterLayerInterface {
        let layer: MCTiled2dMapRasterLayerInterface?
        if let shader {
            layer = create(withShader: config, loaders: loaders, shader: shader)
        } else {
            layer = create(config, loaders: loaders)
        }

        guard let layer else {
            throw Errors.createFailed
        }

        layer.setCallbackHandler(callbackHandler)
        return layer
    }

    static func webMercator(
        _ layerName: String = UUID().uuidString,
        urlFormat: String,
        minZoomLevel: Int = 0,
        maxZoomLevel: Int = 20,
        loaders: [MCLoaderInterface] = [MCTextureLoader()],
        callbackHandler: MCTiled2dMapRasterLayerCallbackInterface? = nil
    ) throws -> MCTiled2dMapRasterLayerInterface {
        guard
            let config = MCDefaultTiled2dMapLayerConfigs.webMercatorCustom(
                layerName,
                urlFormat: urlFormat,
                zoomInfo: nil,
                minZoomLevel: Int32(minZoomLevel),
                maxZoomLevel: Int32(maxZoomLevel)
            )
        else {
            throw Errors.invalidWebMercatorConfig
        }

        return try make(
            config: config,
            loaders: loaders,
            callbackHandler: callbackHandler
        )
    }

    private enum Errors: Error {
        case createFailed
        case invalidWebMercatorConfig
    }
}

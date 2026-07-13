//
//  MCWmtsCapabilitiesResource+Convenience.swift
//
//
//  Created by OpenAI Codex on 05.05.2026.
//

import Foundation

public extension MCWmtsCapabilitiesResource {
    static func from(xmlString: String) throws -> MCWmtsCapabilitiesResource {
        guard let resource = create(xmlString) else {
            throw Errors.capabilitiesLoadFailed
        }
        return resource
    }

    @available(iOS 15.0, *)
    static func from(url: URL) async throws -> MCWmtsCapabilitiesResource {
        let (data, _) = try await URLSession.shared.data(from: url)
        guard let xml = String(data: data, encoding: .utf8) else {
            throw Errors.stringDecodeFailed
        }
        return try from(xmlString: xml)
    }

    func rasterLayer(
        identifier: String,
        tileLoaders: [MCLoaderInterface] = [MCTextureLoader()]
    ) throws -> MCTiled2dMapRasterLayerInterface {
        guard let layer = createLayer(identifier, tileLoaders: tileLoaders) else {
            throw Errors.createLayerFailed
        }
        return layer
    }

    private enum Errors: Error {
        case capabilitiesLoadFailed
        case createLayerFailed
        case stringDecodeFailed
    }
}

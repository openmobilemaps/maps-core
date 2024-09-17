//
//  CapabilitiesResource.swift
//  Meteo
//
//  Created by Nicolas Märki on 01.09.22.
//

import Foundation

@available(iOS 13.0, *)
public class CapabilitiesResource {
    public init(xmlString: String) throws {
        resource = try MCWmtsCapabilitiesResource.create(xmlString) !! Errors.capabilitiesLoadFailed
    }

    @available(iOS 15.0, *)
    public init(url: URL) async throws {
        let (data, _) = try await URLSession.shared.data(from: url)
        let xml = try String(data: data, encoding: .utf8) !! Errors.stringDecodeFailed
        resource = try MCWmtsCapabilitiesResource.create(xml) !! Errors.capabilitiesLoadFailed
    }

    public func createLayer(identifier: String, tileLoaders: [MCLoaderInterface] = [MCTextureLoader()]) throws -> TiledRasterLayer {
        let layer = try resource.createLayer(identifier, tileLoaders: tileLoaders) !! Errors.createLayerFailed
        return TiledRasterLayer(layer)
    }

    enum Errors: Error {
        case capabilitiesLoadFailed
        case createLayerFailed
        case stringDecodeFailed

        var errorCode: String {
            switch self {
                case .capabilitiesLoadFailed:
                    return "CAPLF"
                case .createLayerFailed:
                    return "CLAYF"
                case .stringDecodeFailed:
                    return "STRDF"
            }
        }
    }

    let resource: MCWmtsCapabilitiesResource
}

//
//  VectorStyle+Extensions.swift
//  MapCore
//
//  Created by Nicolas Märki on 22.02.2025.
//

import Foundation

extension VectorStyle {
    init(_ string: String) throws {
        guard let data = string.data(using: .utf8) else {
            throw NSError(domain: "Invalid input string", code: 0, userInfo: nil)
        }
        self = try JSONDecoder().decode(VectorStyle.self, from: data)
    }

    init(url: String) async throws {
        guard let url = URL(string: url) else {
            throw NSError(domain: "Invalid URL string", code: 0, userInfo: nil)
        }
        let data = try await URLSession.shared.data(from: url).0
        self = try JSONDecoder().decode(VectorStyle.self, from: data)
    }
}

extension VectorStyle {
    func filtered(removeLayers: [String]? = nil, keepLayers: [String]? = nil) -> VectorStyle {
        var copy = self
        if let removeLayers {
            copy.layers.removeAll(where: { removeLayers.contains($0.id) })
        }
        if let keepLayers {
            copy.layers.removeAll(where: { !keepLayers.contains($0.id) })
        }
        return copy
    }
}

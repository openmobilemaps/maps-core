//
//  MapLayer.swift
//
//
//  Created by OpenAI Codex on 05.05.2026.
//

import Foundation

public protocol MapLayer {
    func asLayerInterface() -> MCLayerInterface?
}

extension MCTiled2dMapRasterLayerInterface: MapLayer {}
extension MCTiled2dMapVectorLayerInterface: MapLayer {}

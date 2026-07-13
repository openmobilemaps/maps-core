//
//  MCMapInterface+Layers.swift
//
//
//  Created by OpenAI Codex on 04.05.2026.
//

import Foundation

public extension MCMapInterface {
    func addLayer(_ layer: any MapLayer) {
        addLayer(layer.asLayerInterface())
    }

    func insertLayer(_ layer: any MapLayer, at index: Int) {
        insertLayer(at: layer.asLayerInterface(), at: Int32(index))
    }

    func insertLayer(_ layer: any MapLayer, above otherLayer: MCLayerInterface?) {
        insertLayer(above: layer.asLayerInterface(), above: otherLayer)
    }

    func insertLayer(_ layer: any MapLayer, above otherLayer: any MapLayer) {
        insertLayer(above: layer.asLayerInterface(), above: otherLayer.asLayerInterface())
    }

    func insertLayer(_ layer: any MapLayer, below otherLayer: MCLayerInterface?) {
        insertLayer(below: layer.asLayerInterface(), below: otherLayer)
    }

    func insertLayer(_ layer: any MapLayer, below otherLayer: any MapLayer) {
        insertLayer(below: layer.asLayerInterface(), below: otherLayer.asLayerInterface())
    }

    func removeLayer(_ layer: any MapLayer) {
        removeLayer(layer.asLayerInterface())
    }
}

//
//  Test.swift
//  MapCore
//
//  Created by Nicolas Märki on 22.02.2025.
//

import Foundation
import Testing

@MainActor
@Suite(.serialized)
struct BasemapLayerPerformanceTests {

    @Test func testFPSStability() async throws {
        let view = TestingMapView(DataProvider("https://vectortiles.geo.admin.ch/styles/ch.swisstopo.basemap.vt/style.json"))

        try await view.prepare(.zurich)

        let fps = (0..<100).map { _ in view.measureFPS(duration: 5) }
        let maxFps = fps.max()!
        print(maxFps)
        #expect(maxFps > 80.0)
    }

    @Test func testStyleParsing() async throws {
        let style = try await VectorStyle(url: "https://vectortiles.geo.admin.ch/styles/ch.swisstopo.basemap.vt/style.json")
        let provider = DataProvider(style)
        let view = TestingMapView(provider)
        try await view.prepare(.zurich)
        let img = view.currentDrawableImage()
        #expect(img != nil)
    }

    @Test func measureLayerPerformance() async throws {
        let style = try await VectorStyle(url: "https://vectortiles.geo.admin.ch/styles/ch.swisstopo.basemap.vt/style.json")
        var layerFps: [String: Double] = [:]
        for layer in style.layers {
            try await Task {
                let provider = DataProvider(style.filtered(removeLayers: [layer.id]))
                let view = TestingMapView(provider)
                try await view.prepare(.zurich)
                let fps = (0..<5).map {
                    print(layer.id, $0)
                    let fps = view.measureFPS()
                    print(fps)
                    let progress = Double($0 + layerFps.count) / Double(style.layers.count * 5)
                    print("Progress: \(Int(progress * 100))%")
                    return fps
                }
                layerFps[layer.id] = fps.max()!
                print(layer.id, layerFps[layer.id]!)
            }.value
        }
        print(layerFps.sorted { $0.value < $1.value })
    }

}

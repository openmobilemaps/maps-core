//
//  Test.swift
//  MapCore
//
//  Created by Nicolas Märki on 22.02.2025.
//

import Foundation
import Testing

@MainActor
struct BasemapLayerPerformanceTests {

    @Test func testFPSStability() async throws {
        let view = TestingMapView(DataProvider("https://vectortiles.geo.admin.ch/styles/ch.swisstopo.basemap.vt/style.json"))

        try await view.prepare(.zurich)

        let fps = (0..<5).map { _ in view.measureFPS() }

        print(fps)
    }

    @Test func testStyleParsing() async throws {
        let style = try await VectorStyle(url: "https://vectortiles.geo.admin.ch/styles/ch.swisstopo.basemap.vt/style.json")
        let provider = DataProvider(style)
        let view = TestingMapView(provider)
        try await view.prepare(.zurich)
        let _ = view.currentDrawableImage()
    }

    @Test func testMovingCamera() async throws {
        let style = try await VectorStyle(url: "https://vectortiles.geo.admin.ch/styles/ch.swisstopo.basemap.vt/style.json")
        let provider = DataProvider(style)
        let view = TestingMapView(provider)
        for region in TestRegion.all {
            try await view.prepare(region)
            let fps = view.measureFPS()
            print("\(region.identifier): \(fps)")
        }
    }

    @Test func measureLayerPerformance() async throws {
        let style = try await VectorStyle(url: "https://vectortiles.geo.admin.ch/styles/ch.swisstopo.basemap.vt/style.json")
        for layer in style.layers {
            try await Task {
                let provider = DataProvider(style.filtered(keepLayers: [layer.id]))
                let view = TestingMapView(provider)
                try await view.prepare(.zurich)
                let fps = view.measureFPS()
                print("\(layer.id): \(fps)")
            }.value
        }
    }

}

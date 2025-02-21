//
//  Test.swift
//  MapCore
//
//  Created by Nicolas Märki on 19.02.2025.
//

import MapCore
import SnapshotTesting
import Testing

@MainActor
struct BasemapRenderingTests {

    /*
     https://vectortiles.geo.admin.ch/styles/ch.swisstopo.basemap.vt/style.json

     https://vectortiles.geo.admin.ch/styles/ch.swisstopo.lightbasemap.vt/style.json

     https://vectortiles.geo.admin.ch/styles/ch.swisstopo.imagerybasemap.vt/style.json

     https://vectortiles.geo.admin.ch/styles/ch.swisstopo.basemap-winter.vt/style.json
     */

    @Test
    func basicRendering() async throws {
        let view = TestingMapView(baseStyleURL: "https://vectortiles.geo.admin.ch/styles/ch.swisstopo.basemap.vt/style.json")

        try await view.prepare(.zurich)

        view.drawMeasured()
    }

    @Test(arguments: TestRegion.all)
    func basemap(of region: TestRegion) async throws {
        let view = TestingMapView(baseStyleURL: "https://vectortiles.geo.admin.ch/styles/ch.swisstopo.basemap.vt/style.json")

        try await view.prepare(region)

        let img = try #require(view.currentDrawableImage())

        withSnapshotTesting(diffTool: .ksdiff) {
            assertSnapshot(of: img, as: .image(precision: 0.95, perceptualPrecision: 0.95), named: region.identifier)
        }
    }

    @Test(arguments: TestRegion.all)
    func stableRendering(of region: TestRegion) async throws {
        let view = TestingMapView(baseStyleURL: "https://vectortiles.geo.admin.ch/styles/ch.swisstopo.basemap.vt/style.json")

        try await view.prepare(region)

        view.drawMeasured()
    }

    @Test(.disabled("Too slow to be run regularly"), arguments: TestRegion.all)
    func veryStableRendering(of region: TestRegion) async throws {
        for _ in 0..<10 {
            let view = TestingMapView(baseStyleURL: "https://vectortiles.geo.admin.ch/styles/ch.swisstopo.basemap.vt/style.json")

            try await view.prepare(region)

            view.drawMeasured()
        }
    }

}

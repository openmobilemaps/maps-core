//
//  Test.swift
//  MapCore
//
//  Created by Nicolas Märki on 24.02.2025.
//

import SnapshotTesting
import Testing

@MainActor
@Suite(.snapshots(record: .missing, diffTool: .ksdiff), .serialized)
struct BasemapRenderingVariantsTests {

    @Test(arguments: [TestRegion.zurich, .aletsch])
    func testLightBasemapRendering(of region: TestRegion) async throws {
        let view = TestingMapView(DataProvider("https://vectortiles.geo.admin.ch/styles/ch.swisstopo.lightbasemap.vt/style.json"))

        try await view.prepare(region)

        let img = try #require(view.currentDrawableImage())

        withSnapshotTesting(diffTool: .ksdiff) {
            assertSnapshot(of: img, as: .image(precision: 0.95, perceptualPrecision: 0.95), named: region.identifier)
        }
    }

    @Test(arguments: [TestRegion.zurich, .aletsch])
    func testImageryBasemapRendering(of region: TestRegion) async throws {
        let view = TestingMapView(DataProvider("https://vectortiles.geo.admin.ch/styles/ch.swisstopo.imagerybasemap.vt/style.json"))

        try await view.prepare(region)

        let img = try #require(view.currentDrawableImage())

        assertSnapshot(of: img, as: .image(precision: 0.95, perceptualPrecision: 0.95), named: region.identifier, record: true)
    }

    @Test(arguments: [TestRegion.zurich, .aletsch])
    func testWinterBasemapRendering(of region: TestRegion) async throws {
        let view = TestingMapView(DataProvider("https://vectortiles.geo.admin.ch/styles/ch.swisstopo.basemap-winter.vt/style.json"))

        try await view.prepare(region)

        let img = try #require(view.currentDrawableImage())

        assertSnapshot(of: img, as: .image(precision: 0.95, perceptualPrecision: 0.95), named: region.identifier)
    }

}

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
@Suite(.snapshots(record: .missing, diffTool: .ksdiff), .serialized)
struct BasemapRenderingTests {



    @Test(arguments: TestRegion.all)
    func testBasemapRegionRendering(of region: TestRegion) async throws {
        let view = TestingMapView(DataProvider("https://vectortiles.geo.admin.ch/styles/ch.swisstopo.basemap.vt/style.json"))

        try await view.prepare(region)

        let img = try #require(view.currentDrawableImage())

        assertSnapshot(of: img, as: .image(precision: 0.95, perceptualPrecision: 0.95), named: region.identifier)
    }

    

}

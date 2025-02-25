//
//  Test.swift
//  MapCore
//
//  Created by Nicolas Märki on 24.02.2025.
//

import Testing

@MainActor
@Suite(.serialized)
struct BasemapStabilityTests {

    @Test(arguments: TestRegion.all)
    func testStableRendering(of region: TestRegion) async throws {
        let view = TestingMapView(DataProvider("https://vectortiles.geo.admin.ch/styles/ch.swisstopo.basemap.vt/style.json"))

        try await view.prepare(region)

        view.drawMeasured()
    }

    @Test
    func testMapDeallocation() async throws {
        for _ in 0..<20 {
            try await Task {
                let view = TestingMapView(DataProvider("https://vectortiles.geo.admin.ch/styles/ch.swisstopo.basemap.vt/style.json"))

                try await view.prepare(.zurich)

                view.drawMeasured()
            }.value
        }
    }

    @Test(arguments: TestRegion.all)
    func testLayerRemoval(of region: TestRegion) async throws {
        let view = TestingMapView()

        for _ in 0..<20 {

            let layer = view.add(vectorLayer: DataProvider("https://vectortiles.geo.admin.ch/styles/ch.swisstopo.basemap.vt/style.json"))

            try await view.prepare(region)

            view.drawMeasured()

            view.remove(layer: layer)
        }
    }

    @Test
    func testMovingCamera() async throws {
        let provider = DataProvider("https://vectortiles.geo.admin.ch/styles/ch.swisstopo.basemap.vt/style.json")
        let view = TestingMapView(provider)
        for region in TestRegion.all {
            try await Task {
                try await view.prepare(region)
                let fps = view.measureFPS()
            }.value
        }
    }

    @Test(.disabled("Too slow to be run regularly"), arguments: TestRegion.all)
    func testVeryStableRendering(of region: TestRegion) async throws {
        for _ in 0..<10 {
            let view = TestingMapView(DataProvider("https://vectortiles.geo.admin.ch/styles/ch.swisstopo.basemap.vt/style.json"))

            try await view.prepare(region)

            view.drawMeasured()
        }
    }

}

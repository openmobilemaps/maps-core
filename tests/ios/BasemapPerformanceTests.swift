//
//  BasemapPerformanceTests.swift
//  MapCore
//
//  Created by Nicolas Märki on 21.02.2025.
//

import XCTest
import MapCore

@MainActor
final class BasemapPerformanceTests: XCTestCase {

    func testDrawPerformance() async throws {

        let view = TestingMapView(DataProvider("https://vectortiles.geo.admin.ch/styles/ch.swisstopo.basemap.vt/style.json"))

        try await view.prepare(.aletsch)

        self.measure(metrics: [FPSMetric(mapView: view)]) {
            view.drawMeasured()
        }
    }

}

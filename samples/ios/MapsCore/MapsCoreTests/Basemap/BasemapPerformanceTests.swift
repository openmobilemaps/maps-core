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

        try await view.prepare(.zurich)

        let options = XCTMeasureOptions.default
        options.iterationCount = 10
        self.measure(metrics: [FPSMetric(mapView: view)], options: options) {
            view.drawMeasured()
        }
    }

}

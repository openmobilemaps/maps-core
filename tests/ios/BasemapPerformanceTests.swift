//
//  BasemapPerformanceTests.swift
//  MapCore
//
//  Created by Nicolas Märki on 21.02.2025.
//

import XCTest
import MapCore

final class BasemapPerformanceTests: XCTestCase {


    @MainActor
    func testDrawPerformance() async throws {

        let view = TestingMapView(baseStyleURL: "https://vectortiles.geo.admin.ch/styles/ch.swisstopo.basemap.vt/style.json")

        try await view.prepare(.zurich)

        self.measure {
            view.drawMeasured()
        }
    }

}

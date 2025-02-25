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

        let prepareMetric = XCTOSSignpostMetric(subsystem: TestingMapView.signposterSubsystem, category: TestingMapView.signposterCategory, name: "\(TestingMapView.signposterIntervalPrepare)")
        let drawMetric = XCTOSSignpostMetric(subsystem: TestingMapView.signposterSubsystem, category: TestingMapView.signposterCategory, name: "\(TestingMapView.signposterIntervalDraw)")
        let awaitMetric = XCTOSSignpostMetric(subsystem: TestingMapView.signposterSubsystem, category: TestingMapView.signposterCategory, name: "\(TestingMapView.signposterIntervalAwait)")
        self.measure(metrics: [prepareMetric, drawMetric, awaitMetric]) {
            view.drawMeasured(frames: 70)
        }
    }

}

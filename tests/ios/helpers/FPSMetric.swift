//
//  FPSMetric.swift
//  MapCore
//
//  Created by Nicolas Märki on 25.02.2025.
//

import XCTest

class FPSMetric: NSObject, XCTMetric {

    var mapView: TestingMapView

    init(mapView: TestingMapView) {
        self.mapView = mapView
    }

    func reportMeasurements(from startTime: XCTPerformanceMeasurementTimestamp, to endTime: XCTPerformanceMeasurementTimestamp) throws -> [XCTPerformanceMeasurement] {
        let frames = self.mapView.frames.count { $0.start.absoluteTimeNanoSeconds > startTime.absoluteTimeNanoSeconds && $0.end.absoluteTimeNanoSeconds < endTime.absoluteTimeNanoSeconds }
        let duration = Double(endTime.absoluteTimeNanoSeconds - startTime.absoluteTimeNanoSeconds) / Double(1_000_000_000)
        let fps = Double(frames) / duration
        return [.init(identifier: "FPS", displayName: "Frames per Second", value: .init(value: fps, unit: .init(symbol: "fps")), polarity: .prefersLarger)]
    }

    func copy(with zone: NSZone? = nil) -> Any {
        let copy = FPSMetric(mapView: mapView)
        return copy
    }


}

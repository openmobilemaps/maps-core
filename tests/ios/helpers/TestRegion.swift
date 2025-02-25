//
//  Bounds.swift
//  MapCore
//
//  Created by Nicolas Märki on 21.02.2025.
//

import MapCore
import CoreLocation

struct TestRegion {
    let identifier: String
    let topLeft: CLLocationCoordinate2D
    let bottomRight: CLLocationCoordinate2D
}

extension TestRegion {
    var mcBounds: MCRectCoord {
        .init(
            topLeft: .init(lat: topLeft.latitude, lon: topLeft.longitude),
            bottomRight: .init(lat: bottomRight.latitude, lon: bottomRight.longitude)
        )
    }
}

extension TestRegion: CustomDebugStringConvertible {
    var debugDescription: String {
        identifier
    }
}

extension TestRegion {

    static let zurich = TestRegion(identifier: "Zürich", topLeft: .init(latitude: 47.434, longitude: 8.488), bottomRight: .init(latitude: 47.322, longitude: 8.631))

    static let aletsch = TestRegion(identifier: "Aletsch Glacier", topLeft: .init(latitude: 46.535, longitude: 8.093), bottomRight: .init(latitude: 46.387, longitude: 8.135))

    static let all: [TestRegion] = [
        .zurich, // City
        TestRegion(identifier: "Central", topLeft: .init(latitude: 47.380188, longitude: 8.539151), bottomRight: .init(latitude: 47.373003, longitude: 8.549014)), // Neighbourhood
        TestRegion(identifier: "Geneva", topLeft: .init(latitude: 46.239, longitude: 6.094), bottomRight: .init(latitude: 46.179, longitude: 6.166)), // City
        TestRegion(identifier: "Bern", topLeft: .init(latitude: 46.966, longitude: 7.398), bottomRight: .init(latitude: 46.931, longitude: 7.473)), // City
        TestRegion(identifier: "Zermatt", topLeft: .init(latitude: 46.030, longitude: 7.688), bottomRight: .init(latitude: 45.971, longitude: 7.710)), // Village
        TestRegion(identifier: "Matterhorn", topLeft: .init(latitude: 45.998, longitude: 7.756), bottomRight: .init(latitude: 45.971, longitude: 7.807)), // Mountain
        TestRegion(identifier: "Lake Geneva", topLeft: .init(latitude: 46.500, longitude: 6.000), bottomRight: .init(latitude: 46.200, longitude: 6.780)), // Lake
        .aletsch, // Glacier
        TestRegion(identifier: "Lake Constance", topLeft: .init(latitude: 47.730, longitude: 9.030), bottomRight: .init(latitude: 47.420, longitude: 9.610)), // Lake
        TestRegion(identifier: "Engadin", topLeft: .init(latitude: 46.85, longitude: 9.80), bottomRight: .init(latitude: 46.35, longitude: 10.48)), // Region
        TestRegion(identifier: "Lugano", topLeft: .init(latitude: 46.031, longitude: 8.925), bottomRight: .init(latitude: 45.976, longitude: 8.999)), // City
        TestRegion(identifier: "Appenzell", topLeft: .init(latitude: 47.342, longitude: 9.399), bottomRight: .init(latitude: 47.322, longitude: 9.417))  // Village
    ]
}

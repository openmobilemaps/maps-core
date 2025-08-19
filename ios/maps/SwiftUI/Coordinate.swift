//
//  Coordinate.swift
//  MapCore
//
//  Created by Nicolas Märki on 16.08.2025.
//

public struct Coordinate: Sendable, Codable, Equatable, Hashable, CustomStringConvertible {


    public var x: Double
    public var y: Double
    public var z: Double
    public let systemIdentifier: Int32

    private init(x: Double, y: Double, z: Double, systemIdentifier: Int32) {
        self.x = x
        self.y = y
        self.z = z
        self.systemIdentifier = systemIdentifier
    }

    public init(latitude: Double, longitude: Double, elevation: Double = 0) {
        self.init(x: longitude, y: latitude, z: elevation, systemIdentifier: MCCoordinateSystemIdentifiers.epsg4326())
    }

    public var description: String {
        "(x:\(x), y:\(y))"
    }

}

#if canImport(SwiftUI)
import SwiftUI

extension LocalizedStringKey.StringInterpolation {
    public mutating func appendInterpolation(_ coordinate: Coordinate) {
        appendInterpolation(coordinate.description)
    }
}

#endif

extension Coordinate {
    public static func epsg4326(latitude: Double, longitude: Double, elevation: Double = 0) -> Coordinate {
        .init(x: longitude, y: latitude, z: elevation, systemIdentifier: MCCoordinateSystemIdentifiers.epsg4326())
    }
    public static func epsg3857(x: Double, y: Double, elevation: Double = 0) -> Coordinate {
        .init(x: x, y: y, z: elevation, systemIdentifier: MCCoordinateSystemIdentifiers.epsg3857())
    }
    public static func epsg2056(easting: Double, northing: Double, elevation: Double = 0) -> Coordinate {
        .init(x: easting, y: northing, z: elevation, systemIdentifier: MCCoordinateSystemIdentifiers.epsg2056())
    }
    public static func epsg21781(easting: Double, northing: Double, elevation: Double = 0) -> Coordinate {
        .init(x: easting, y: northing, z: elevation, systemIdentifier: MCCoordinateSystemIdentifiers.epsg21781())
    }
}

extension Coordinate {

    public init(_ coord: MCCoord) {
        self.init(x: coord.x, y: coord.y, z: coord.z, systemIdentifier: coord.systemIdentifier)
    }

    var mcCoord: MCCoord {
        MCCoord(systemIdentifier: systemIdentifier, x: x, y: y, z: z)
    }
}

#if canImport(CoreLocation)
    import CoreLocation

    extension Coordinate {
        public init(_ locationCoordinate: CLLocationCoordinate2D) {
            self.init(x: locationCoordinate.longitude, y: locationCoordinate.latitude, z: 0, systemIdentifier: MCCoordinateSystemIdentifiers.epsg4326())
        }
    }
#endif

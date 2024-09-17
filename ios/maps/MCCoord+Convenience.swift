//
//  MCCoord+Convenience.swift
//
//
//  Created by Nicolas Märki on 14.02.2024.
//

import Foundation
import MapCoreSharedModule

#if canImport(CoreLocation)
import CoreLocation
#endif

public extension MCCoord {
    convenience init(lat: Double, lon: Double) {
        self.init(systemIdentifier: MCCoordinateSystemIdentifiers.epsg4326(), x: lon, y: lat, z: 0)
    }

#if canImport(CoreLocation)
    convenience init(coord: CLLocationCoordinate2D) {
        self.init(lat: coord.latitude, lon: coord.longitude)
    }
#endif
}

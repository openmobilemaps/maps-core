/*
 * Copyright (c) 2021 Ubique Innovation AG <https://www.ubique.ch>
 *
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this
 *  file, You can obtain one at https://mozilla.org/MPL/2.0/.
 *
 *  SPDX-License-Identifier: MPL-2.0
 */

import CoreLocation
import Foundation
import MapCoreSharedModule

public extension CLLocation {
    var mcCoord: MCCoord {
        coordinate.mcCoord
    }
}

public extension CLLocationCoordinate2D {
    var mcCoord: MCCoord {
        MCCoord(systemIdentifier: MCCoordinateSystemIdentifiers.epsg4326(),
                x: longitude,
                y: latitude,
                z: 0)
    }
}

public extension MCCoord {
    var clLocation: CLLocation? {
        guard let coord = clLocationCoordinate else { return nil }
        return CLLocation(latitude: coord.latitude, longitude: coord.longitude)
    }

    var clLocationCoordinate: CLLocationCoordinate2D? {
        guard let coord = MCCoordinateConversionHelperInterface.independentInstance()?.convert(MCCoordinateSystemIdentifiers.epsg4326(), coordinate: self) else { return nil }
        return CLLocationCoordinate2D(latitude: coord.y, longitude: coord.x)
    }
}

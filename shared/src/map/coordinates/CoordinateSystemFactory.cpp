/*
 * Copyright (c) 2021 Ubique Innovation AG <https://www.ubique.ch>
 *
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this
 *  file, You can obtain one at https://mozilla.org/MPL/2.0/.
 *
 *  SPDX-License-Identifier: MPL-2.0
 */

#include "CoordinateSystemFactory.h"
#include "Coord.h"
#include "CoordinateSystemIdentifiers.h"
#include "EPSG4326System2D.h"
#include "MapCoordinateSystem.h"
#include "RectCoord.h"
#include <cmath>
#include <stdexcept>
#include <string>


// The identifier accessors are not constexpr, so they cannot be used as switch case labels.
::MapCoordinateSystem CoordinateSystemFactory::getSystemFor(int32_t identifier) {
    if (identifier == CoordinateSystemIdentifiers::EPSG2056()) {
        return getEpsg2056System();
    } else if (identifier == CoordinateSystemIdentifiers::EPSG3857()) {
        return getEpsg3857System();
    } else if (identifier == CoordinateSystemIdentifiers::EPSG4326()) {
        return getEpsg4326System();
    } else if (identifier == CoordinateSystemIdentifiers::EPSG4326System2D()) {
        return getEpsg4326System2d();
    } else if (identifier == CoordinateSystemIdentifiers::EPSG21781()) {
        return getEpsg21781System();
    } else if (identifier == CoordinateSystemIdentifiers::UnitSphere()) {
        return getUnitSphereSystem();
    } else {
        throw std::invalid_argument("Unsupported coordinate system identifier: " + std::to_string(identifier));
    }
}

::MapCoordinateSystem CoordinateSystemFactory::getEpsg2056System() {
    return MapCoordinateSystem(CoordinateSystemIdentifiers::EPSG2056(),
                               RectCoord(Coord(CoordinateSystemIdentifiers::EPSG2056(), 2420000.0, 1350000.0, 0),
                                         Coord(CoordinateSystemIdentifiers::EPSG2056(), 2900000.0, 1030000.0, 0)),
                               1.0);
}

::MapCoordinateSystem CoordinateSystemFactory::getEpsg3857System() {
    return MapCoordinateSystem(CoordinateSystemIdentifiers::EPSG3857(),
                               RectCoord(Coord(CoordinateSystemIdentifiers::EPSG3857(), -20037508.34, 20037508.34, 0),
                                         Coord(CoordinateSystemIdentifiers::EPSG3857(), 20037508.34, -20037508.34, 0)),
                               1.0);
}

::MapCoordinateSystem CoordinateSystemFactory::getEpsg4326System() {
    return MapCoordinateSystem(CoordinateSystemIdentifiers::EPSG4326(),
                               RectCoord(Coord(CoordinateSystemIdentifiers::EPSG4326(), -180.0 , 90, 0),
                                         Coord(CoordinateSystemIdentifiers::EPSG4326(), 180.0, -90, 0)),
                               (1.0 / (40075017.0 / 360.0)));
}

::MapCoordinateSystem CoordinateSystemFactory::getEpsg4326System2d() {
    constexpr double scaledMaxLatitude = 90.0 * EPSG4326System2d::ScaleY;

    const auto identifier = CoordinateSystemIdentifiers::EPSG4326System2D();
    return MapCoordinateSystem(identifier,
                               RectCoord(Coord(identifier, -180.0, scaledMaxLatitude, 0),
                                         Coord(identifier, 180.0, -scaledMaxLatitude, 0)),
                               (1.0 / (40075017.0 / 360.0)));
}

::MapCoordinateSystem CoordinateSystemFactory::getEpsg21781System() {
    return MapCoordinateSystem(CoordinateSystemIdentifiers::EPSG21781(),
                               RectCoord(Coord(CoordinateSystemIdentifiers::EPSG21781(), 485000.0, 300000.0, 0),
                                         Coord(CoordinateSystemIdentifiers::EPSG21781(), 840000.0, 70000.0, 0)),
                               1.0);
}

::MapCoordinateSystem CoordinateSystemFactory::getUnitSphereSystem() {
    return MapCoordinateSystem(CoordinateSystemIdentifiers::UnitSphere(),
                               RectCoord(Coord(CoordinateSystemIdentifiers::UnitSphere(), -2.0 * M_PI, 0.0,         0.0),
                                         Coord(CoordinateSystemIdentifiers::UnitSphere(),  0.0       , -1.0 * M_PI, 3.0)),
                               1.0 / (40075017.0 / (2 * M_PI)));
}

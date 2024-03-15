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
#include "MapCoordinateSystem.h"
#include "RectCoord.h"

::MapCoordinateSystem CoordinateSystemFactory::getEpsg2056System() {
    return MapCoordinateSystem(CoordinateSystemIdentifiers::EPSG2056(),
                               RectCoord(Coord(CoordinateSystemIdentifiers::EPSG2056(), 2485000.0, 1300000.0, 0),
                                         Coord(CoordinateSystemIdentifiers::EPSG2056(), 2840000.0, 1070000.0, 0)),
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

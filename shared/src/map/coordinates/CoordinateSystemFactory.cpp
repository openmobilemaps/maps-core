//
// Created by Christoph Maurhofer on 03.02.2021.
//

#include "CoordinateSystemFactory.h"
#include "Coord.h"
#include "CoordinateSystemIdentifiers.h"
#include "MapCoordinateSystem.h"
#include "RectCoord.h"

::MapCoordinateSystem CoordinateSystemFactory::getEpsg2056System() {
    return MapCoordinateSystem(CoordinateSystemIdentifiers::EPSG2056(),
                               RectCoord(Coord(CoordinateSystemIdentifiers::EPSG2056(), 2485071.58, 1299941.79, 0),
                                         Coord(CoordinateSystemIdentifiers::EPSG2056(), 2828515.82, 1075346.31, 0)),
                               1.0);
}

::MapCoordinateSystem CoordinateSystemFactory::getEpsg3857System() {
    return MapCoordinateSystem(CoordinateSystemIdentifiers::EPSG3857(),
                               RectCoord(Coord(CoordinateSystemIdentifiers::EPSG3857(), -20026376.39, 20048966.10, 0),
                                         Coord(CoordinateSystemIdentifiers::EPSG3857(), 20026376.39, -20048966.10, 0)),
                               1.0);
}

::MapCoordinateSystem CoordinateSystemFactory::getEpsg21781System() {
    return MapCoordinateSystem(CoordinateSystemIdentifiers::EPSG21781(),
                               RectCoord(Coord(CoordinateSystemIdentifiers::EPSG21781(), 485071.54, 75346.36, 0),
                                         Coord(CoordinateSystemIdentifiers::EPSG21781(), 828515.78, 299941.84, 0)),
                               1.0);



}

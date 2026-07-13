/*
 * Copyright (c) 2021 Ubique Innovation AG <https://www.ubique.ch>
 *
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this
 *  file, You can obtain one at https://mozilla.org/MPL/2.0/.
 *
 *  SPDX-License-Identifier: MPL-2.0
 */

#pragma once

#include "Coord.h"
#include "CoordinateConversionHelper.h"
#include "CoordinateConverterInterface.h"
#include "CoordinateSystemIdentifiers.h"
#include "MapCoordinateSystem.h"
#include <cmath>

/// Convert WGS 84 / Pseudo-Mercator to LV03
/// https://epsg.io/3857 to https://epsg.io/2056
class EPSG3857ToEPSG2056Converter : public CoordinateConverterInterface {
public:
    EPSG3857ToEPSG2056Converter() {}

    virtual Coord convert(const Coord& coordinate) override {
        static constexpr double invHalfEarth = M_PI / 20037508.34;
        static constexpr double lngDegScale = 180.0 / 20037508.34;
        static constexpr double degToArcSec = 3600.0;

        const double latDeg = std::atan(std::exp(coordinate.y * invHalfEarth)) * (360.0 / M_PI) - 90.0;
        const double lngDeg = coordinate.x * lngDegScale;
        const double lat = latDeg * degToArcSec;
        const double lng = lngDeg * degToArcSec;

        // Axiliary values (% Bern)
        const double lat_aux = (lat - 169028.66) / 10000.;
        const double lng_aux = (lng - 26782.5) / 10000.;

        const double x = (600072.37 + 211455.93 * lng_aux - 10938.51 * lng_aux * lat_aux - 0.36 * lng_aux * (lat_aux * lat_aux) -
                   44.54 * (lng_aux * lng_aux * lng_aux)) + 2000000;

        const double y = (200147.07 + 308807.95 * lat_aux + 3745.25 * lng_aux * lng_aux + 76.63 * lat_aux * lat_aux -
                   194.56 * lng_aux * lng_aux * lat_aux + 119.79 * lat_aux * lat_aux * lat_aux) + 1000000;

        const double z = coordinate.z - 49.55 + 2.73 * lng_aux + 6.94 * lat_aux;

        return Coord(getTo(), x, y, z);
    }

    virtual int32_t getFrom() override { return CoordinateSystemIdentifiers::EPSG3857(); }

    virtual int32_t getTo() override { return CoordinateSystemIdentifiers::EPSG2056(); }
};

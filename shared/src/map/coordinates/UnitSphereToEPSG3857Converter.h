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

/// Convert WGS 84 / Pseudo-Mercator to WGS84
/// https://epsg.io/3857 to https://epsg.io/4326
class UnitSphereToEPSG3857Converter : public CoordinateConverterInterface {
public:
    UnitSphereToEPSG3857Converter() {}

    virtual Coord convert(const Coord &coordinate) override {

        const double EarthRadius = 6378137.0; // Earth's radius in meters (EPSG:3857 uses this value)

        double x3D = coordinate.x;
        double y3D = coordinate.y;
        double z3D = coordinate.z;

        // Calculate latitude and longitude in radians
        double longitude = atan2(y3D, x3D);
        double latitude = asin(z3D);

        // Convert latitude and longitude to EPSG3857 coordinates
        double x = EarthRadius * longitude;
        double y = EarthRadius * log(tan(M_PI_4 + latitude / 2.0));

        return Coord(
                     CoordinateSystemIdentifiers::EPSG3857(),
                     x3D, y3D, 0.0);

    }

    virtual std::string getFrom() override { return CoordinateSystemIdentifiers::UNITSPHERE(); }

    virtual std::string getTo() override { return CoordinateSystemIdentifiers::EPSG3857(); }
};

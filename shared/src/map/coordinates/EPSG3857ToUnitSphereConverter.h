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

/// Convert WGS 84 / Pseudo-Mercator to Unit Sphere
/// https://epsg.io/3857 to Unit Sphere x,y,z in [-1, 1]
class EPSG3857ToUnitSphereConverter : public CoordinateConverterInterface {
public:
    EPSG3857ToUnitSphereConverter() {}

    virtual Coord convert(const Coord &coordinate) override {

        const double EarthRadius = 6378137.0; // Earth's radius in meters (EPSG:3857 uses this value)

        double x = coordinate.x;
        double y = coordinate.y;

        /*

        // Calculate latitude and longitude in radians
        double longitude = x / EarthRadius;
        double latitude = 2 * atan(sinh(y / EarthRadius)) + M_PI_2;

        // Convert latitude and longitude to 3D Cartesian coordinates on a unit sphere
        double sinLat = sin(latitude);
        double cosLat = cos(latitude);
        double sinLon = sin(longitude);
        double cosLon = cos(longitude);

        double phi = latitude;
        double lambda = longitude;

        double x3D = sin(phi) * cos(lambda);
        double y3D = cos(-phi);
        double z3D = sin(-phi)*sin(lambda);

         */



        // Calculate latitude and longitude in radians
        double longitude = x / EarthRadius;
        double latitude = 2 * atan(exp(y / EarthRadius)) - M_PI_2;

//        double xx = (coordinate.x * 180 / 20037508.34) / 180 * M_2_PI;
//        double yy = (atan(exp(coordinate.y * M_PI / 20037508.34)) * 360 / M_PI - 90) / 180 * M_2_PI;
//
//        longitude = xx;
//        latitude = yy;

        // Convert latitude and longitude to 3D Cartesian coordinates on a unit sphere
        double sinLat = sin(latitude);
        double cosLat = cos(latitude);
        double sinLon = sin(longitude);
        double cosLon = cos(longitude);

        double x3D = cosLat * cosLon;
        double y3D = cosLat * sinLon;
        double z3D = sinLat;


        return Coord(CoordinateSystemIdentifiers::UNITSPHERE(),
                     x3D, y3D, z3D);

    }

    virtual std::string getFrom() override { return CoordinateSystemIdentifiers::EPSG3857(); }

    virtual std::string getTo() override { return CoordinateSystemIdentifiers::UNITSPHERE(); }
};

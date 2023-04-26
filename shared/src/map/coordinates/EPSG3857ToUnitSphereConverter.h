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

/// Convert WGS 84  to Unit Sphere
/// https://epsg.io/3857 to Unit Sphere x,y,z in [-1, 1]
class EPSG3857ToUnitSphereConverter : public CoordinateConverterInterface {
public:
    EPSG3857ToUnitSphereConverter() {}

    virtual Coord convert(const Coord &coordinate) override {

        const double EarthRadius = 6378137.0; // Earth's radius in meters (EPSG:3857 uses this value)

        double x = coordinate.x * 180 / 20037508.34;
        double y = atan(exp(coordinate.y * M_PI / 20037508.34)) * 360 / M_PI - 90;

        float longitude = x / 180.0 * M_PI + M_PI; // [0, 2pi]
        float latitude = y / 90.0 * M_PI - M_PI; // [-2pi, 0]

        float sinLon = sin(longitude);     // [0, 1, 0, -1, 0]
        float cosLon = cos(longitude);     // [1, 0, -1, 0, 1]
        float sinLatH = sin(latitude / 2); // [0, 1, 0, -1, 0]
        float cosLatH = cos(latitude / 2); // [1, 0, -1, 0, 1]

        // scale by altitude, 0 = on unit sphere
        float alt = 1.0 + coordinate.z / EarthRadius;

        float x3D = sinLon * sinLatH * alt;
        float y3D = cosLatH * alt;
        float z3D = cosLon * sinLatH * alt;


        return Coord(CoordinateSystemIdentifiers::UNITSPHERE(),
                     x3D, y3D, z3D);

    }

    virtual std::string getFrom() override { return CoordinateSystemIdentifiers::EPSG3857(); }

    virtual std::string getTo() override { return CoordinateSystemIdentifiers::UNITSPHERE(); }
};

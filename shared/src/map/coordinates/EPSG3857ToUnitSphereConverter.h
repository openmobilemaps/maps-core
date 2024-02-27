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

// Convert EPSG:3857 coordinates to the unit sphere in polar coordinates (0/0/EARTH_RADIUS maps to 0/0/1)
class EPSG3857ToUnitSphereConverter : public CoordinateConverterInterface {
public:
    EPSG3857ToUnitSphereConverter() {}

    virtual Coord convert(const Coord &coordinate) override {

        const double lon = (coordinate.x * 180.0 / 20037508.34);
        const double lat = atan(exp(coordinate.y * M_PI / 20037508.34)) * 360.0 / M_PI - 90.0;

        const double phi = (lon - 180.0) * M_PI / 180.0; // [-2 * pi, 0)
        const double th = (lat - 90.0) * M_PI / 180.0; // [0, -pi]
        const double r = 1.0 + coordinate.z / 6378137.0;

        return Coord(getTo(), phi, th, r);
    }

    virtual int32_t getFrom() override { return CoordinateSystemIdentifiers::EPSG3857(); }

    virtual int32_t getTo() override { return CoordinateSystemIdentifiers::UnitSphere(); }
};

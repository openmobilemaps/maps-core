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

// Convert EPSG:4326 coordinates to the unit sphere in cartesian coordinates (0/0/EARTH_RADIUS maps to 1/0/0)
class EPSG4326ToUnitSphereCartConverter : public CoordinateConverterInterface {
public:
    EPSG4326ToUnitSphereCartConverter() {}

    virtual Coord convert(const Coord &coordinate) override {

        const double phi = (coordinate.x - 180.0) * M_PI / 180.0; // [-2 * pi, 0)
        const double th = (coordinate.y - 90.0) * M_PI / 180.0;
        const double r = 1.0 + coordinate.z / 6378137.0;

        return Coord(getTo(), phi, th, r);
    }

    virtual int32_t getFrom() override { return CoordinateSystemIdentifiers::EPSG4326(); }

    virtual int32_t getTo() override { return CoordinateSystemIdentifiers::UnitSphereCart(); }
};

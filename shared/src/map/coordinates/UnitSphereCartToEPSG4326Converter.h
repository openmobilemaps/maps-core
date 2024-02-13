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

// Convert cartesian unit sphere coordinates to EPSG:4326
class UnitSphereCartToEPSG4326Converter : public CoordinateConverterInterface {
public:
    UnitSphereCartToEPSG4326Converter() {}

    virtual Coord convert(const Coord &coordinate) override {

        const double r = sqrt(coordinate.x * coordinate.x + coordinate.y * coordinate.y + coordinate.z * coordinate.z);
        const double th = -acos(coordinate.y / r);
        const double phi = ((coordinate.z < 0.0) - (coordinate.z > 0.0)) * acos(coordinate.x / sqrt(coordinate.x * coordinate.x + coordinate.z * coordinate.z));

        const double x = phi * 180.0 / M_PI;
        const double y = th * 180.0 / M_PI + 90.0;
        const double z = r * 6378137.0;

        return Coord(getTo(), x, y, z);
    }

    virtual int32_t getFrom() override { return CoordinateSystemIdentifiers::UnitSphereCart(); }

    virtual int32_t getTo() override { return CoordinateSystemIdentifiers::EPSG4326(); }
};

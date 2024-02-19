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

// Convert cartesian unit sphere coordinates to EPSG:3857
class UnitSphereCartToEPSG3857Converter : public CoordinateConverterInterface {
public:
    UnitSphereCartToEPSG3857Converter() {}

    virtual Coord convert(const Coord &coordinate) override {

        const double r = sqrt(coordinate.x * coordinate.x + coordinate.y * coordinate.y + coordinate.z * coordinate.z);
        const double th = acos(coordinate.y / r);
        const double phi = ((coordinate.z < 0.0) - (coordinate.z > 0.0)) * acos(coordinate.x / sqrt(coordinate.x * coordinate.x + coordinate.z * coordinate.z));

        const double lon = phi * 180.0 / M_PI;
        const double lat = th * 180.0 / M_PI + 90.0;

        const double x = lon * 20037508.34 / 180.0;
        const double y = ((log(tan(((90.0 + lat) * M_PI) / 360.0)) / (M_PI / 180.0)) * 20037508.34) / 180.0;
        const double z = r * 6378137.0;

        return Coord(getTo(), x, y, z);
    }

    virtual int32_t getFrom() override { return CoordinateSystemIdentifiers::UnitSphereCart(); }

    virtual int32_t getTo() override { return CoordinateSystemIdentifiers::EPSG3857(); }
};

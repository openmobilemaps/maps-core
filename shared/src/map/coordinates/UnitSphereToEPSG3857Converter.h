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

// Convert unit sphere polar coordinates to EPSG:3857
class UnitSphereToEPSG3857Converter : public CoordinateConverterInterface {
public:
    UnitSphereToEPSG3857Converter() {}

    virtual Coord convert(const Coord &coordinate) override {
        const double lon = coordinate.x * 180.0 / M_PI;
        const double lat = coordinate.y * 180.0 / M_PI + 90.0;

        const double x = lon * 20037508.34 / 180.0;
        const double y = ((log(tan(((90.0 + lat) * M_PI) / 360.0)) / (M_PI / 180.0)) * 20037508.34) / 180.0;
        const double z = coordinate.z * 6378137.0;

        return Coord(getTo(), x, y, z);
    }

    virtual int32_t getFrom() override { return CoordinateSystemIdentifiers::UnitSphere(); }

    virtual int32_t getTo() override { return CoordinateSystemIdentifiers::EPSG3857(); }
};

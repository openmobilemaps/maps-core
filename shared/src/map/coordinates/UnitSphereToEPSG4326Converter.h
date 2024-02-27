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

// Convert unit sphere polar coordinates to EPSG:4326
class UnitSphereToEPSG4326Converter : public CoordinateConverterInterface {
public:
    UnitSphereToEPSG4326Converter() {}

    virtual Coord convert(const Coord &coordinate) override {
        const double x = coordinate.x * 180.0 / M_PI;
        const double y = coordinate.y * 180.0 / M_PI + 90.0;
        const double z = coordinate.z * 6378137.0;

        return Coord(getTo(), x, y, z);
    }

    virtual int32_t getFrom() override { return CoordinateSystemIdentifiers::UnitSphere(); }

    virtual int32_t getTo() override { return CoordinateSystemIdentifiers::EPSG4326(); }
};

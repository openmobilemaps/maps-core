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
class EPSG3857ToEPSG4326Converter : public CoordinateConverterInterface {
  public:
    EPSG3857ToEPSG4326Converter() {}

    virtual Coord convert(const Coord &coordinate) override {

        double x = coordinate.x * 180 / 20037508.34;
        double y = atan(exp(coordinate.y * M_PI / 20037508.34)) * 360 / M_PI - 90;

        return Coord(getTo(), x, y, 0);
    }

    virtual std::string getFrom() override { return CoordinateSystemIdentifiers::EPSG3857(); }

    virtual std::string getTo() override { return CoordinateSystemIdentifiers::EPSG4326(); }
};

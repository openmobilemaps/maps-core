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

/// Convert WGS84 to WGS 84 / Pseudo-Mercator
///  https://epsg.io/4326 to https://epsg.io/3857
class EPSG4326ToEPSG3857Converter : public CoordinateConverterInterface {
  public:
    EPSG4326ToEPSG3857Converter() {}

    virtual Coord convert(const Coord &coordinate) override {

        double x = coordinate.x * 20037508.34 / 180;
        double y = log(tan(((90 + coordinate.y) * M_PI) / 360)) / (M_PI / 180);
        y = (y * 20037508.34) / 180;

        return Coord(getTo(), x, y, 0);
    }

    virtual std::string getFrom() override { return CoordinateSystemIdentifiers::EPSG4326(); }

    virtual std::string getTo() override { return CoordinateSystemIdentifiers::EPSG3857(); }
};

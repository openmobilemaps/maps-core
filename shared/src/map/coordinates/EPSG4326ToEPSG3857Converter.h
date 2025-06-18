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
#include <algorithm>

/// Convert WGS84 to WGS 84 / Pseudo-Mercator
///  https://epsg.io/4326 to https://epsg.io/3857
class EPSG4326ToEPSG3857Converter : public CoordinateConverterInterface {
  public:
    EPSG4326ToEPSG3857Converter() {}

    virtual Coord convert(const Coord &coordinate) override {

        const double x = coordinate.x * 20037508.34 / 180;
        const double y = ((log(tan(((90 + std::clamp(coordinate.y, -85.06, 85.06)) * M_PI) / 360)) / (M_PI / 180)) * 20037508.34) / 180;
        return Coord(getTo(), x, y, coordinate.z);
    }

    virtual int32_t getFrom() override { return CoordinateSystemIdentifiers::EPSG4326(); }

    virtual int32_t getTo() override { return CoordinateSystemIdentifiers::EPSG3857(); }
};

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

/// Convert (new, prefixed) LV03+ to (old, not-prefixed) LV03
/// https://epsg.io/2056 to https://epsg.io/21781
class EPSG2056ToEPGS21781Converter : public CoordinateConverterInterface {
  public:
    EPSG2056ToEPGS21781Converter() {}

    virtual Coord convert(const Coord &coordinate) override {

        const double x = coordinate.x - 2000000;
        const double y = coordinate.y - 1000000;

        return Coord(getTo(), x, y, coordinate.z);
    }

    virtual int32_t getFrom() override { return CoordinateSystemIdentifiers::EPSG2056(); }

    virtual int32_t getTo() override { return CoordinateSystemIdentifiers::EPSG21781(); }
};

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

/// Convert (old, not-prefixed) LV03 to (new, prefixed) LV03+
///  https://epsg.io/21781 to https://epsg.io/2056
class EPSG21781ToEPGS2056Converter : public CoordinateConverterInterface {
public:
    EPSG21781ToEPGS2056Converter() {}

    virtual Coord convert(const Coord &coordinate) override {

        double x = coordinate.x + 2000000;
        double y = coordinate.y + 1000000;

        return Coord(getTo(), x, y, 0);
    }

    virtual std::string getFrom() override { return CoordinateSystemIdentifiers::EPSG21781(); }

    virtual std::string getTo() override { return CoordinateSystemIdentifiers::EPSG2056(); }
};

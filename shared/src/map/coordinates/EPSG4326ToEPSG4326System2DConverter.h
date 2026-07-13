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
#include "CoordinateConverterInterface.h"
#include "CoordinateSystemIdentifiers.h"
#include "EPSG4326System2D.h"

/// Convert WGS84 to Fluid's 2D WGS84 display coordinate system.
class EPSG4326ToEPSG4326System2DConverter : public CoordinateConverterInterface {
  public:
    EPSG4326ToEPSG4326System2DConverter() {}

    virtual Coord convert(const Coord &coordinate) override {
        return Coord(getTo(), coordinate.x, coordinate.y * EPSG4326System2d::ScaleY, coordinate.z);
    }

    virtual int32_t getFrom() override { return CoordinateSystemIdentifiers::EPSG4326(); }

    virtual int32_t getTo() override { return CoordinateSystemIdentifiers::EPSG4326System2D(); }
};

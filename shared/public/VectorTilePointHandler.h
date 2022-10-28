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
#include "RectCoord.h"
#include "vtzero/geometry.hpp"

class VectorTilePointHandler
{
  public:
    VectorTilePointHandler(::RectCoord tileCoords, int extent) : tileCoords(tileCoords), extent((double) extent), tileWidth(tileCoords.bottomRight.x - tileCoords.topLeft.x), tileHeight(tileCoords.bottomRight.y - tileCoords.topLeft.y) {};

    const Coord getPoint() {
        return data[0];
    }

    std::vector<Coord> getPoints() {
        return data;
    }

    // Handling of decoding

    void points_begin(uint32_t count) {
        data.reserve(count);
    }

    void points_point(const vtzero::point point) {
        double x = tileCoords.topLeft.x + tileWidth * (point.x / extent);
        double y = tileCoords.topLeft.y + tileHeight * (point.y / extent);
        Coord newCoord = Coord(tileCoords.topLeft.systemIdentifier, x, y, 0.0);

        data.push_back(newCoord);
    }

    void points_end() const noexcept {}

  private:
    std::vector<Coord> data;
    
    RectCoord tileCoords;
    double tileWidth;
    double tileHeight;
    double extent;
};

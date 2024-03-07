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
#include "RectCoord.h"
#include "vtzero/geometry.hpp"

class VectorTilePolygonHandler {
public:
    VectorTilePolygonHandler(::RectCoord tileCoords, int extent) : tileCoords(tileCoords), extent((double) extent),
                                                                   tileWidth(tileCoords.bottomRight.x - tileCoords.topLeft.x),
                                                                   tileHeight(tileCoords.bottomRight.y - tileCoords.topLeft.y) {};

    void ring_begin(uint32_t count) {
        currentRing = std::vector<::Coord>();
        currentRing.reserve(count);
    }

    void ring_point(vtzero::point point) noexcept {
        double x = tileCoords.topLeft.x + tileWidth * (point.x / extent);
        double y = tileCoords.topLeft.y + tileHeight * (point.y / extent);
        Coord newCoord = Coord(tileCoords.topLeft.systemIdentifier, x, y, 0.0);
        currentRing.push_back(newCoord);
    }

    void ring_end(vtzero::ring_type ringType) noexcept {
        if (!currentRing.empty()) {
            currentRing.push_back(Coord(currentRing[0]));

            switch (ringType) {
                case vtzero::ring_type::outer:
                    coordinates.push_back(currentRing);
                    holes.push_back(std::vector<std::vector<::Coord>>());
                    break;
                case vtzero::ring_type::inner:
                    holes.back().push_back(currentRing);
                    break;
                case vtzero::ring_type::invalid:
                    currentRing.clear();
                    break;
            }
        }
    }

    std::vector<std::vector<::Coord>> getCoordinates() {
        return std::move(coordinates);
    }

    std::vector<std::vector<std::vector<::Coord>>> getHoles() {
        return std::move(holes);
    }

private:
    std::vector<::Coord> currentRing;
    std::vector<std::vector<::Coord>> coordinates;
    std::vector<std::vector<std::vector<::Coord>>> holes;

    RectCoord tileCoords;
    double tileWidth;
    double tileHeight;
    double extent;
};

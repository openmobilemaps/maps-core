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

class VectorTileLineHandler {
public:

    VectorTileLineHandler(::RectCoord tileCoords, int extent) : tileCoords(tileCoords), extent((double) extent),
                                                                   tileWidth(tileCoords.bottomRight.x - tileCoords.topLeft.x),
                                                                   tileHeight(tileCoords.bottomRight.y - tileCoords.topLeft.y) {};

    void points_begin(const uint32_t count) {}

    void points_point(const vtzero::point point) {}

    void points_end() {}

    void linestring_begin(const uint32_t count) {
        featureIndex++;
        coordinates.push_back({});
        coordinates.at(featureIndex).reserve(count);
    }

    void linestring_point(const vtzero::point point) {
        double x = tileCoords.topLeft.x + tileWidth * (point.x / extent);
        double y = tileCoords.topLeft.y + tileHeight * (point.y / extent);
        Coord newCoord = Coord(tileCoords.topLeft.systemIdentifier, x, y, 0.0);
        coordinates.at(featureIndex).push_back(newCoord);
    }

    void linestring_end() {}

    void ring_begin(const uint32_t count) {
        featureIndex++;
        coordinates.push_back({});
        coordinates.reserve(count);
    }

    void ring_point(const vtzero::point point) {
        double x = tileCoords.topLeft.x + tileWidth * (point.x / extent);
        double y = tileCoords.topLeft.y + tileHeight * (point.y / extent);
        Coord newCoord = Coord(tileCoords.topLeft.systemIdentifier, x, y, 0.0);
        coordinates.at(featureIndex).push_back(newCoord);
    }

    void ring_end(const vtzero::ring_type rt) {}

    std::vector<std::vector<::Coord>> getCoordinates() {
        return std::move(coordinates);
    }

    void reset() {
        coordinates.clear();
        featureIndex = -1;
    }

private:
    int featureIndex = -1;
    std::vector<std::vector<::Coord>> coordinates;

    RectCoord tileCoords;
    double tileWidth;
    double tileHeight;
    double extent;

};

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

class VectorTileGeometryHandler {
public:
    VectorTileGeometryHandler(::RectCoord tileCoords, int extent) : tileCoords(tileCoords), extent((double) extent),
    minX(std::min(tileCoords.topLeft.x, tileCoords.bottomRight.x)),
    minY(std::min(tileCoords.topLeft.y, tileCoords.bottomRight.y)),
    tileWidth(std::abs(tileCoords.bottomRight.x - tileCoords.topLeft.x)),
    tileHeight(std::abs(tileCoords.bottomRight.y - tileCoords.topLeft.y)),
    topToBottom (tileCoords.topLeft.y < tileCoords.bottomRight.y) {};

    void points_begin(const uint32_t count) {
        currentFeature = std::vector<::Coord>();
        currentFeature.reserve(count);
    }

    void points_point(const vtzero::point point) {
        double x = minX + tileWidth * (point.x / extent);
        double y = minY + tileHeight * ( topToBottom ?  (point.y / extent) : ( 1 - (point.y / extent)) ) ;
        Coord newCoord = Coord(tileCoords.topLeft.systemIdentifier, x, y, 0.0);
        currentFeature.push_back(newCoord);
    }

    void points_end() {
        coordinates.push_back(currentFeature);
        currentFeature.clear();
    }

    void linestring_begin(const uint32_t count) {
        currentFeature = std::vector<::Coord>();
        currentFeature.reserve(count);
    }

    void linestring_point(const vtzero::point point) {
        double x = minX + tileWidth * (point.x / extent);
        double y = minY + tileHeight * ( topToBottom ?  (point.y / extent) : ( 1 - (point.y / extent)) ) ;
        Coord newCoord = Coord(tileCoords.topLeft.systemIdentifier, x, y, 0.0);
        currentFeature.push_back(newCoord);
    }

    void linestring_end() {
        coordinates.push_back(currentFeature);
        currentFeature.clear();
    }

    void ring_begin(uint32_t count) {
        currentFeature = std::vector<::Coord>();
        currentFeature.reserve(count);
    }

    void ring_point(vtzero::point point) noexcept {
        double x = minX + tileWidth * (point.x / extent);
        double y = minY + tileHeight * ( topToBottom ?  (point.y / extent) : ( 1 - (point.y / extent)) ) ;
        Coord newCoord = Coord(tileCoords.topLeft.systemIdentifier, x, y, 0.0);
        currentFeature.push_back(newCoord);
    }

    void ring_end(vtzero::ring_type ringType) noexcept {
        if (!currentFeature.empty()) {
            currentFeature.push_back(Coord(currentFeature[0]));

            switch (ringType) {
                case vtzero::ring_type::outer:
                    coordinates.push_back(currentFeature);
                    holes.push_back(std::vector<std::vector<::Coord>>());
                    break;
                case vtzero::ring_type::inner:
                    holes.back().push_back(currentFeature);
                    break;
                case vtzero::ring_type::invalid:
                    currentFeature.clear();
                    break;
            }
            currentFeature.clear();
        }
    }

    std::vector<std::vector<::Coord>> getLineCoordinates() const {
        std::vector<std::vector<::Coord>> lineCoordinates;
        lineCoordinates.insert(lineCoordinates.end(), coordinates.begin(), coordinates.end());
        for (const auto &hole : holes) {
            lineCoordinates.insert(lineCoordinates.end(), hole.begin(), hole.end());
        }
        return std::move(lineCoordinates);
    }

    const std::vector<std::vector<::Coord>> getPolygonCoordinates() const {
        return coordinates;
    }

    const std::vector<std::vector<std::vector<::Coord>>> getHoleCoordinates() const {
        return holes;
    }

    const std::vector<std::vector<::Coord>> getPointCoordinates() const {
        return coordinates;
    }

    void reset() {
        currentFeature.clear();
        coordinates.clear();
        holes.clear();
    }

private:
    std::vector<::Coord> currentFeature;
    std::vector<std::vector<::Coord>> coordinates;
    std::vector<std::vector<std::vector<::Coord>>> holes;

    RectCoord tileCoords;
    double minX;
    double minY;
    double tileWidth;
    double tileHeight;
    double extent;
    bool topToBottom;
};

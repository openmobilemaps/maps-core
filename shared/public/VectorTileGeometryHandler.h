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
#include "Tiled2dMapVectorSettings.h"
#include "vtzero/geometry.hpp"
#include "Logger.h"

class VectorTileGeometryHandler {
public:
    VectorTileGeometryHandler(::RectCoord tileCoords, int extent, const std::optional<Tiled2dMapVectorSettings> &vectorSettings)
    : tileCoords(tileCoords),
      // use standard TOP_LEFT origin, when no vector settings given.
      origin(vectorSettings ? vectorSettings->tileOrigin : Tiled2dMapVectorTileOrigin::TOP_LEFT),
      extent((double)extent)
    {};

    void points_begin(const uint32_t count) {
        currentFeature = std::vector<::Coord>();
        currentFeature.reserve(count);
    }

    void points_point(const vtzero::point point) {
        currentFeature.emplace_back(coordinateFromPoint(point));
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
        currentFeature.emplace_back(coordinateFromPoint(point));
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
        currentFeature.emplace_back(coordinateFromPoint(point));
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

    void limitHoles(uint32_t maxHoles) {
        for (auto &polygonHoles: holes) {
            if (polygonHoles.size() > maxHoles) {
                std::nth_element(polygonHoles.begin(),
                                 polygonHoles.begin() + maxHoles,
                                 polygonHoles.end(),
                                 [](const auto &a, const auto &b) {
                                     return std::fabs(signedArea(a)) > std::fabs(signedArea(b));
                                 });
                polygonHoles.resize(maxHoles);
            }
        }
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
    inline Coord coordinateFromPoint(const vtzero::point &point) {
        auto tx = point.x / extent;
        auto ty = point.y / extent;

        switch(origin) {
            case Tiled2dMapVectorTileOrigin::TOP_LEFT: {
                break;
            }
            case Tiled2dMapVectorTileOrigin::BOTTOM_LEFT: {
                ty = 1.0 - ty; break;
            }
            case Tiled2dMapVectorTileOrigin::TOP_RIGHT: {
                tx = 1.0 - tx; break;
            }
            case Tiled2dMapVectorTileOrigin::BOTTOM_RIGHT: {
                tx = 1.0 - tx; ty = 1.0 - ty; break;
            }
        }
//
//        auto x = tileCoords.topLeft.x * (1.0 - tx) + tileCoords.bottomRight.x * tx;
//        auto y = tileCoords.topLeft.y * (1.0 - ty) + tileCoords.bottomRight.y * ty;


        auto x = -1 * (1.0 - tx) + 1 * tx;
        auto y = -1 * (1.0 - ty) + 1 * ty;

        return Coord(tileCoords.topLeft.systemIdentifier, x, y, 0.0);
    }

    static double signedArea(const std::vector<::Coord>& hole) {
        double sum = 0;
        for (std::size_t i = 0, len = hole.size(), j = len - 1; i < len; j = i++) {
            const ::Coord& p1 = hole[i];
            const ::Coord& p2 = hole[j];
            sum += (p2.x - p1.x) * (p1.y + p2.y);
        }
        return sum;
    }

private:
    std::vector<::Coord> currentFeature;
    std::vector<std::vector<::Coord>> coordinates;
    std::vector<std::vector<std::vector<::Coord>>> holes;

    Tiled2dMapVectorTileOrigin origin;
    RectCoord tileCoords;
    double extent;
};

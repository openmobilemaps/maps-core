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
#include "earcut.hpp"
#include "Logger.h"
#include "CoordinateConversionHelperInterface.h"
#include "CoordinateSystemIdentifiers.h"


class VectorTileGeoJsonGeometryHandler {
public:
    VectorTileGeometryHandler(const std::shared_ptr<CoordinateConversionHelperInterface> &conversionHelper)
    : conversionHelper(conversionHelper)
    {};

    VectorTileGeometryHandler(const VectorTileGeometryHandler& other) = delete;

    void points_begin(const uint32_t count) {
        currentFeature = std::vector<::Coord>();
        currentFeature.reserve(count);
    }

    void points_point(const vtzero::point point) {
        currentFeature.emplace_back(coordinateFromPoint(point, false));
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
        currentFeature.emplace_back(coordinateFromPoint(point, false));
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
        currentFeature.emplace_back(point);
    }

    void ring_end(vtzero::ring_type ringType) noexcept {
        if (!currentFeature.empty()) {
            coordinates.push_back(polygonCurrentRing[0]);

            switch (ringType) {
                case vtzero::ring_type::outer:
                    polygonPoints.push_back(polygonCurrentRing);
                    polygonHoles.push_back(std::vector<std::vector<vtzero::point>>());
                    break;
                case vtzero::ring_type::inner:
                    polygonHoles.back().push_back(polygonCurrentRing);
                    break;
                case vtzero::ring_type::invalid:
                    polygonCurrentRing.clear();
                    break;
            }
            polygonCurrentRing.clear();
        }
    }

    const std::vector<std::vector<::Coord>> &getCoordinates() {
        return coordinates;
    }

private:

    std::vector<::Coord> currentFeature;
    std::vector<std::vector<::Coord>> coordinates;
    std::vector<std::vector<::Coord>> holes;

    const std::shared_ptr<CoordinateConversionHelperInterface> conversionHelper;
};

/*
 * Copyright (c) 2021 Ubique Innovation AG <https://www.ubique.ch>
 *
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this
 *  file, You can obtain one at https://mozilla.org/MPL/2.0/.
 *
 *  SPDX-License-Identifier: MPL-2.0
 */

#include "PolygonHelper.h"

bool PolygonHelper::pointInside(const PolygonInfo &polygon, const Coord &point,
                                const std::shared_ptr<CoordinateConversionHelperInterface> &conversionHelper) {
    return pointInside(point, polygon.coordinates, polygon.holes, conversionHelper);
}

bool PolygonHelper::pointInside(const Coord &point, const std::vector<Coord> &positions,
                                const std::vector<std::vector<Coord>> holes,
                                const std::shared_ptr<CoordinateConversionHelperInterface> &conversionHelper) {

    // check if in polygon
    bool inside = pointInside(point, positions, conversionHelper);

    for (auto &hole : holes) {
        if (pointInside(point, hole, conversionHelper)) {
            inside = false;
            break;
        }
    }

    return inside;
}

bool PolygonHelper::pointInside(const Coord &point, const std::vector<Coord> &positions,
                                const std::shared_ptr<CoordinateConversionHelperInterface> &conversionHelper) {

    bool c = false;

    size_t nvert = positions.size();

    auto pointsystemIdentifier = point.systemIdentifier;
    auto x = point.x;
    auto y = point.y;

    std::vector<Coord> convertedPositions;
    for (auto const &position : positions) {
        convertedPositions.push_back(conversionHelper->convert(pointsystemIdentifier, position));
    }

    size_t i, j;
    for (i = 0, j = nvert - 1; i < nvert; j = i++) {
        auto ypi = convertedPositions[i].y;
        auto ypj = convertedPositions[j].y;

        auto xpi = convertedPositions[i].x;
        auto xpj = convertedPositions[j].x;

        if ((((ypi <= y) && (y < ypj)) || ((ypj <= y) && (y < ypi))) && (x < (xpj - xpi) * (y - ypi) / (ypj - ypi) + xpi)) {
            c = !c;
        }
    }

    return c;
}

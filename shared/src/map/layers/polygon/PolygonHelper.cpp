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


bool PolygonHelper::pointInside(const PolygonCoord &polygon, const Coord &point,
                        const std::shared_ptr<CoordinateConversionHelperInterface> &conversionHelper) {
    return pointInside(point, polygon.positions, polygon.holes, conversionHelper);
}

bool PolygonHelper::pointInside(const PolygonInfo &polygon, const Coord &point,
                                const std::shared_ptr<CoordinateConversionHelperInterface> &conversionHelper) {
    return pointInside(point, polygon.coordinates.positions, polygon.coordinates.holes, conversionHelper);
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

std::vector<::PolygonCoord> PolygonHelper::clip(const PolygonCoord &a, const PolygonCoord &b, const ClippingOperation operation) {
    gpc_polygon a_, b_, result_;
    gpc_set_polygon(a, &a_);
    gpc_set_polygon(b, &b_);
    gpc_polygon_clip(gpcOperationFrom(operation), &a_, &b_, &result_);
    auto result = gpc_get_polygon_coord(&result_, a.positions.begin()->systemIdentifier);
    gpc_free_polygon(&a_);
    gpc_free_polygon(&b_);
    gpc_free_polygon(&result_);
    return result;
}

gpc_op PolygonHelper::gpcOperationFrom(const ClippingOperation operation) {
    switch (operation) {
        case Intersection:
            return GPC_INT;
        case Difference:
            return GPC_DIFF;
        case Union:
            return GPC_UNION;
        case XOR:
            return GPC_XOR;
    }
}

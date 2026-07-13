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

#include "QuadCoord.h"
#include "CoordinateConversionHelperInterface.h"
#include "CoordinateSystemFactory.h"
#include "CoordinateSystemIdentifiers.h"
#include "MapCoordinateSystem.h"
#include "RectCoord.h"
#include <algorithm>
#include <cassert>
#include <cmath>
#include <vector>

namespace coordsutil {

    inline RectCoord normalizeRectCoord(const RectCoord &rect) {
        assert(rect.topLeft.systemIdentifier == rect.bottomRight.systemIdentifier);
        return RectCoord(Coord(rect.topLeft.systemIdentifier, std::min(rect.topLeft.x, rect.bottomRight.x),
                               std::min(rect.topLeft.y, rect.bottomRight.y), 0.0),
                         Coord(rect.topLeft.systemIdentifier, std::max(rect.topLeft.x, rect.bottomRight.x),
                               std::max(rect.topLeft.y, rect.bottomRight.y), 0.0));
    }

    inline RectCoord intersectRectCoords2d(const RectCoord &r1, const RectCoord &r2) {
        assert(r1.topLeft.systemIdentifier == r2.topLeft.systemIdentifier);
        const auto a = normalizeRectCoord(r1);
        const auto b = normalizeRectCoord(r2);
        return RectCoord(
            Coord(a.topLeft.systemIdentifier, std::max(a.topLeft.x, b.topLeft.x), std::max(a.topLeft.y, b.topLeft.y), 0.0),
            Coord(a.topLeft.systemIdentifier, std::min(a.bottomRight.x, b.bottomRight.x),
                  std::min(a.bottomRight.y, b.bottomRight.y), 0.0));
    }

    inline bool checkRectCoordHasArea2d(const RectCoord &rect, double epsilon = 0.0) {
        return rect.bottomRight.x - rect.topLeft.x > epsilon && rect.bottomRight.y - rect.topLeft.y > epsilon;
    }

    inline double rectCoordArea2d(const RectCoord &rect) {
        return (rect.bottomRight.x - rect.topLeft.x) * (rect.bottomRight.y - rect.topLeft.y);
    }

    // Coordinate systems covering the full world are periodic in x: distances take the shorter way
    // around the antimeridian. For regional systems the bounds width is a data extent, not a period.
    inline bool isFullWorldSystemWrappingX(int32_t systemIdentifier) {
        return systemIdentifier == CoordinateSystemIdentifiers::EPSG3857() ||
               systemIdentifier == CoordinateSystemIdentifiers::EPSG4326() ||
               systemIdentifier == CoordinateSystemIdentifiers::EPSG4326System2D() ||
               systemIdentifier == CoordinateSystemIdentifiers::UnitSphere();
    }

    inline double closestDistanceToRectCoord2d(double x, double y, const RectCoord &rect) {
        double wrapWidthX = 0.0;
        if (isFullWorldSystemWrappingX(rect.topLeft.systemIdentifier)) {
            const auto system = CoordinateSystemFactory::getSystemFor(rect.topLeft.systemIdentifier);
            wrapWidthX = std::abs(system.bounds.bottomRight.x - system.bounds.topLeft.x);
        }

        const auto normalized = normalizeRectCoord(rect);
        const double closestY = std::clamp(y, normalized.topLeft.y, normalized.bottomRight.y);
        auto distanceForX = [&](double sampleX) {
            const double closestX = std::clamp(sampleX, normalized.topLeft.x, normalized.bottomRight.x);
            return std::hypot(closestX - sampleX, closestY - y);
        };

        double distance = distanceForX(x);
        if (wrapWidthX > 0.0) {
            distance = std::min(distance, distanceForX(x - wrapWidthX));
            distance = std::min(distance, distanceForX(x + wrapWidthX));
        }
        return distance;
    }

    // coveringRects must not overlap in area; otherwise intersection areas would be double-counted.
    inline bool checkRectCoordFullyCoveredByNonOverlappingRects2d(
        const RectCoord &rect,
        const std::vector<RectCoord> &coveringRects,
        double epsilon = 1e-12) {

        const auto normalizedRect = normalizeRectCoord(rect);
        if (!checkRectCoordHasArea2d(normalizedRect, epsilon)) {
            return true;
        }

        const double rectArea = rectCoordArea2d(normalizedRect);
        double coveredArea = 0.0;

        for (const auto &cover : coveringRects) {
            const auto normalizedCover = normalizeRectCoord(cover);
            if (!checkRectCoordHasArea2d(normalizedCover, epsilon)) {
                continue;
            }

            const auto intersection = intersectRectCoords2d(normalizedRect, normalizedCover);
            if (!checkRectCoordHasArea2d(intersection, epsilon)) {
                continue;
            }

            coveredArea += rectCoordArea2d(intersection);
            if (coveredArea + epsilon >= rectArea) {
                return true;
            }
        }

        return false;
    }

    inline bool checkIntersectionRectCoords2d(const RectCoord &r1, const RectCoord &r2) {
        assert(r1.topLeft.systemIdentifier == r2.topLeft.systemIdentifier);
        const auto intersection = intersectRectCoords2d(r1, r2);
        return intersection.bottomRight.x >= intersection.topLeft.x && intersection.bottomRight.y >= intersection.topLeft.y;
    }
   
    // @returns true iff coord is inside rect (borders inclusive)
    inline bool checkRectContainsCoord(const RectCoord &rect, const Coord &coord) {
        assert(rect.topLeft.systemIdentifier == rect.bottomRight.systemIdentifier);
        assert(rect.topLeft.systemIdentifier == coord.systemIdentifier);

        auto maxX = std::max(rect.topLeft.x, rect.bottomRight.x);
        auto minX = std::min(rect.topLeft.x, rect.bottomRight.x);
        auto maxY = std::max(rect.topLeft.y, rect.bottomRight.y);
        auto minY = std::min(rect.topLeft.y, rect.bottomRight.y);

        return coord.x >= minX && coord.x <= maxX && coord.y >= minY && coord.y <= maxY;
    }

    inline bool checkRectContainsCoord(const RectCoord &rect, const Coord &coord, const std::shared_ptr<CoordinateConversionHelperInterface> &conversionHelper) {
        assert(rect.topLeft.systemIdentifier == rect.bottomRight.systemIdentifier);
        
        auto convCoord = conversionHelper->convert(rect.topLeft.systemIdentifier, coord);
        return checkRectContainsCoord(rect, convCoord);
    }
}

Coord operator-(const Coord &c1, const Coord &c2);

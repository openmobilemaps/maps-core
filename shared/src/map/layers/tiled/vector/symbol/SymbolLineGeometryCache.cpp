/*
 * Copyright (c) 2021 Ubique Innovation AG <https://www.ubique.ch>
 *
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this
 *  file, You can obtain one at https://mozilla.org/MPL/2.0/.
 *
 *  SPDX-License-Identifier: MPL-2.0
 */

#include "SymbolLineGeometryCache.h"
#include "Matrix.h"
#include "TrigonometryLUT.h"
#include <algorithm>
#include <cmath>

std::shared_ptr<SymbolLineGeometryCache> SymbolLineGeometryCache::create(const std::shared_ptr<CoordinateConversionHelperInterface> &converter,
                                                                         const std::vector<Vec2D> &lineCoordinates,
                                                                         int32_t systemIdentifier,
                                                                         bool is3d) {
    if (lineCoordinates.empty()) {
        return nullptr;
    }

    auto cache = std::shared_ptr<SymbolLineGeometryCache>(new SymbolLineGeometryCache());
    cache->is3d = is3d;
    cache->tileLineCoordinates = lineCoordinates;
    cache->renderLineCoordinates.reserve(lineCoordinates.size());
    for (const auto &coordinate : lineCoordinates) {
        cache->renderLineCoordinates.push_back(
            Vec3DHelper::toVec(converter->convertToRenderSystem(Vec2DHelper::toCoord(coordinate, systemIdentifier))));
    }

    if (is3d) {
        cache->buildCartesianVertices();
    }

    cache->screenLineCoordinates = cache->renderLineCoordinates;
    return cache;
}

void SymbolLineGeometryCache::buildCartesianVertices() {
    cartesianRenderLineCoordinates.clear();
    cartesianRenderLineCoordinates.reserve(renderLineCoordinates.size());

    for (const auto &coordinate : renderLineCoordinates) {
        double sinX, cosX, sinY, cosY;
        lut::sincos(coordinate.y, sinY, cosY);
        lut::sincos(coordinate.x, sinX, cosX);

        cartesianRenderLineCoordinates.emplace_back(coordinate.z * sinY * cosX,
                                                    coordinate.z * cosY,
                                                    -coordinate.z * sinY * sinX);
    }
}

void SymbolLineGeometryCache::ensureScreenProjection3D(const std::vector<float> &vpMatrix, const Vec3D &origin, const Vec2I &viewportSize) {
    if (!is3d || renderLineCoordinates.empty()) {
        return;
    }

    if (screenProjectionValid &&
        lastViewportSize.x == viewportSize.x &&
        lastViewportSize.y == viewportSize.y &&
        lastProjectionOrigin == origin &&
        lastVpMatrix == vpMatrix) {
        return;
    }

    lastViewportSize = viewportSize;
    lastProjectionOrigin = origin;
    lastVpMatrix = vpMatrix;
    screenProjectionValid = true;

    for (size_t i = 0; i < cartesianRenderLineCoordinates.size(); ++i) {
        const auto &cartesian = cartesianRenderLineCoordinates[i];
        const auto &projected = Matrix::multiply(vpMatrix, Vec4D(cartesian.x - origin.x, cartesian.y - origin.y, cartesian.z - origin.z, 1.0));

        screenLineCoordinates[i].x = projected.x * viewportSize.x / 2.0 + viewportSize.x / 2.0;
        screenLineCoordinates[i].y = viewportSize.y / 2.0 - projected.y * viewportSize.y / 2.0;
    }
}

size_t SymbolLineGeometryCache::mapArrayIndex(size_t arrayIndex, bool reversed) const {
    if (!reversed || renderLineCoordinates.empty()) {
        return arrayIndex;
    }
    return renderLineCoordinates.size() - 1 - arrayIndex;
}

Vec2D SymbolLineGeometryCache::pointOnSegment(const std::vector<Vec3D> &coordinates, int segmentIndex, double percentage, bool reversed) const {
    const size_t count = coordinates.size();
    if (count == 0) {
        return Vec2D(0.0, 0.0);
    }

    const int nextIndex = segmentIndex + 1 < (int)count ? segmentIndex + 1 : segmentIndex;
    const auto &start = coordinates[mapArrayIndex((size_t)segmentIndex, reversed)];
    const auto &end = coordinates[mapArrayIndex((size_t)nextIndex, reversed)];
    return Vec2D(start.x + (end.x - start.x) * percentage,
                 start.y + (end.y - start.y) * percentage);
}

Vec2D SymbolLineGeometryCache::pointOnTileSegment(int segmentIndex, double percentage, bool reversed) const {
    const size_t count = tileLineCoordinates.size();
    if (count == 0) {
        return Vec2D(0.0, 0.0);
    }

    const int nextIndex = segmentIndex + 1 < (int)count ? segmentIndex + 1 : segmentIndex;
    const auto &start = tileLineCoordinates[mapArrayIndex((size_t)segmentIndex, reversed)];
    const auto &end = tileLineCoordinates[mapArrayIndex((size_t)nextIndex, reversed)];
    return Vec2D(start.x + (end.x - start.x) * percentage,
                 start.y + (end.y - start.y) * percentage);
}

Vec2D SymbolLineGeometryCache::renderPointAt(int segmentIndex, double percentage, bool reversed) const {
    return pointOnSegment(renderLineCoordinates, segmentIndex, percentage, reversed);
}

Vec2D SymbolLineGeometryCache::screenPointAt(int segmentIndex, double percentage, bool reversed) const {
    return pointOnSegment(screenLineCoordinates, segmentIndex, percentage, reversed);
}

Vec2D SymbolLineGeometryCache::tilePointAt(int segmentIndex, double percentage, bool reversed) const {
    return pointOnTileSegment(segmentIndex, percentage, reversed);
}

LineSegmentIndex SymbolLineGeometryCache::findReferencePoint(const Vec3D &point, bool reversed) const {
    if (screenLineCoordinates.size() < 2) {
        return LineSegmentIndex(0, 0.0);
    }

    auto distance = std::numeric_limits<double>::max();
    const auto point2D = Vec2D(point.x, point.y);

    double tMin = 0.0;
    int iMin = 0;

    const size_t count = screenLineCoordinates.size();
    for (size_t i = 1; i < count; ++i) {
        const auto start = screenPointAt((int)i - 1, 0.0, reversed);
        const auto end = screenPointAt((int)i, 0.0, reversed);

        const auto lengthSquared = Vec2DHelper::distanceSquared(start, end);

        double t = 0.0;
        double dist = 0.0;

        if (lengthSquared > 0) {
            const auto endMinusStart = Vec2D(end.x - start.x, end.y - start.y);
            t = Vec2D(point.x - start.x, point.y - start.y) * endMinusStart / lengthSquared;

            if (t > 1.0) {
                continue;
            }

            const auto proj = Vec2D(start.x + t * endMinusStart.x, start.y + t * endMinusStart.y);
            dist = Vec2DHelper::distanceSquared(proj, point2D);
        } else {
            dist = Vec2DHelper::distanceSquared(start, point2D);
        }

        if (dist < distance) {
            tMin = t;
            iMin = (int)i - 1;
            distance = dist;
        }
    }

    return LineSegmentIndex(iMin, tMin);
}

void SymbolLineGeometryCache::indexAtDistance(const LineSegmentIndex &index, const Vec2D &currentPoint, double distance, bool reversed, LineSegmentIndex &result) const {
    auto dist = std::abs(distance);
    auto current = currentPoint;

    int currentI = index.index;
    double currentPercentage = index.percentage;

    const size_t vertexCount = renderLineCoordinates.size();

    if (distance >= 0) {
        const auto start = std::min(index.index + 1, (int)vertexCount - 1);

        for (int i = start; i < (int)vertexCount; i++) {
            const auto next = screenPointAt(i, 0.0, reversed);

            const double d = Vec2DHelper::distance(current, next);

            if (dist > d) {
                dist -= d;
                current.x = next.x;
                current.y = next.y;
                currentI = i;
                currentPercentage = 0;
            } else {
                result.index = currentI;
                result.percentage = currentPercentage + dist / d * (1.0 - currentPercentage);
                return;
            }
        }
    } else {
        const auto start = index.index;

        for (int i = start; i >= 0; i--) {
            const auto next = screenPointAt(i, 0.0, reversed);

            const double d = Vec2DHelper::distance(current, next);

            if (dist > d) {
                dist -= d;
                current.x = next.x;
                current.y = next.y;

                currentI = i;
                currentPercentage = 0.0;
            } else {
                if (i == currentI) {
                    result.index = i;
                    result.percentage = currentPercentage - currentPercentage * dist / d;
                    return;
                } else {
                    result.index = i;
                    result.percentage = 1.0 - dist / d;
                    return;
                }
            }
        }
    }

    result = LineSegmentIndex(currentI, currentPercentage);
}

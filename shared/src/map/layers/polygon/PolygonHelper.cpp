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
#include "RectCoord.h"
#include "Vec2FHelper.h"
#include <cassert>
#include <queue>

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

PolygonCoord PolygonHelper::coordsFromRect(const RectCoord &rect) {
    return std::move(PolygonCoord(
                                  {rect.topLeft,
                                      Coord(rect.topLeft.systemIdentifier, rect.bottomRight.x,
                                            rect.topLeft.y, 0),
                                      rect.bottomRight,
                                      Coord(rect.topLeft.systemIdentifier, rect.topLeft.x,
                                            rect.bottomRight.y, 0),
                                      rect.topLeft},
                                  {}));
}

// Helper function to find or create a midpoint. Returns 0 as index, if no new vertex is allowed
uint16_t PolygonHelper::findOrCreateMidpoint(std::unordered_map<uint32_t, uint16_t> &midpointCache,
                              std::vector<Vec2F> &vertices,
                              uint16_t v0, uint16_t v1, uint16_t maxVertexCount) {
    // Ensure the smaller index comes first to avoid duplicate edges in different order
    uint32_t smallerIndex = std::min(v0, v1);
    uint32_t largerIndex = std::max(v0, v1);
    uint32_t key = (smallerIndex << 16) | largerIndex;
    
    // Check if midpoint is already created
    auto it = midpointCache.find(key);
    if (it != midpointCache.end()) {
        return it->second;
    }

    if (vertices.size() >= maxVertexCount) {
        return 0;
    }

    // Create new midpoint, normalize it and add to vertex list
    Vec2F midpoint = Vec2FHelper::midpoint(vertices[v0], vertices[v1]);
    uint16_t newIndex = vertices.size();
    assert(newIndex < std::numeric_limits<uint16_t>::max());
    vertices.push_back(midpoint);
    
    // Cache the midpoint
    midpointCache[key] = newIndex;
    return newIndex;
}

void PolygonHelper::subdivision(std::vector<Vec2F> &vertices, std::vector<uint16_t> &indices, float threshold, uint16_t maxVertexCount) {
    std::unordered_map<uint32_t, uint16_t> midpointCache;

    size_t offset = 0;
    while (true) {
        bool subdivided = false;
        size_t currentSize = indices.size();
        for (size_t i = offset; i < currentSize; i += 3) {
            uint16_t v0 = indices[i];
            uint16_t v1 = indices[i + 1];
            uint16_t v2 = indices[i + 2];

            float d0 = Vec2FHelper::distance(vertices[v0], vertices[v1]);
            float d1 = Vec2FHelper::distance(vertices[v1], vertices[v2]);
            float d2 = Vec2FHelper::distance(vertices[v2], vertices[v0]);

            uint16_t a = d0 > threshold ? findOrCreateMidpoint(midpointCache, vertices, v0, v1, maxVertexCount) : 0;
            uint16_t b = d1 > threshold ? findOrCreateMidpoint(midpointCache, vertices, v1, v2, maxVertexCount) : 0;
            uint16_t c = d2 > threshold ? findOrCreateMidpoint(midpointCache, vertices, v2, v0, maxVertexCount) : 0;

            if (a > 0 && b > 0 && c > 0) {
                indices[i + 0] = v0; indices[i + 1] = a; indices[i + 2] = c;
                indices.push_back(v1); indices.push_back(b); indices.push_back(a);
                indices.push_back(v2); indices.push_back(c); indices.push_back(b);
                indices.push_back(a); indices.push_back(b); indices.push_back(c);
                subdivided = true;
            } else if (a > 0 && d0 >= std::max(d1, d2)) {
                indices[i + 0] = v0; indices[i + 1] = a; indices[i + 2] = v2;
                indices.push_back(a); indices.push_back(v1); indices.push_back(v2);
                subdivided = true;
            } else if (b > 0 && d1 >= std::max(d0, d2)) {
                indices[i + 0] = v1; indices[i + 1] = b; indices[i + 2] = v0;
                indices.push_back(b); indices.push_back(v2); indices.push_back(v0);
                subdivided = true;
            } else if (c > 0 && d2 >= std::max(d0, d1)) {
                indices[i + 0] = v2; indices[i + 1] = c; indices[i + 2] = v1;
                indices.push_back(c); indices.push_back(v0); indices.push_back(v1);
                subdivided = true;
            } else {
                // do nothing and increase offset so in next iteration this gets ignored
                if (offset + 1 == i) {
                    offset = i;
                }
            }
        }

        if (!subdivided) {
            break;
        }
    }
}

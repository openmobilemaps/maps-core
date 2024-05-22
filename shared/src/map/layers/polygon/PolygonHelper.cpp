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
#include <unordered_map>

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

// Helper function to find or create a midpoint
uint16_t findOrCreateMidpoint(std::unordered_map<uint32_t, uint16_t> &midpointCache,
                              std::vector<Vec2F> &vertices,
                              uint16_t v0, uint16_t v1) {
    // Ensure the smaller index comes first to avoid duplicate edges in different order
    uint32_t smallerIndex = std::min(v0, v1);
    uint32_t largerIndex = std::max(v0, v1);
    uint32_t key = (smallerIndex << 16) | largerIndex;
    
    // Check if midpoint is already created
    auto it = midpointCache.find(key);
    if (it != midpointCache.end()) {
        return it->second;
    }
    
    // Create new midpoint, normalize it and add to vertex list
    Vec2F midpoint = Vec2FHelper::midpoint(vertices[v0], vertices[v1]);
    uint16_t newIndex = vertices.size();
    vertices.push_back(midpoint);
    
    // Cache the midpoint
    midpointCache[key] = newIndex;
    return newIndex;
}
// Function to recursively subdivide triangles
void PolygonHelper::subdivision(std::vector<Vec2F> &vertices, std::vector<uint16_t> &indices, float threshold, int level) {
    if (level == 0) {
        return;
    }
    std::unordered_map<uint32_t, uint16_t> midpointCache;
    std::vector<uint16_t> newIndices;
    bool subdivided = false;
    
    // Iterate over triangles
    for (size_t i = 0; i < indices.size(); i += 3) {
        uint16_t v0 = indices[i];
        uint16_t v1 = indices[i + 1];
        uint16_t v2 = indices[i + 2];
        
        // Check edge lengths
        
        float d0 = Vec2FHelper::distance(vertices[v0], vertices[v1]);
        float d1 = Vec2FHelper::distance(vertices[v1], vertices[v2]);
        float d2 = Vec2FHelper::distance(vertices[v2], vertices[v0]);

        // Subdivide edges longer than the threshold
        if (d0 > threshold && d1 > threshold && d2 > threshold) {
            // All edges are longer than the threshold, subdivide all edges
            uint16_t a = findOrCreateMidpoint(midpointCache, vertices, v0, v1);
            uint16_t b = findOrCreateMidpoint(midpointCache, vertices, v1, v2);
            uint16_t c = findOrCreateMidpoint(midpointCache, vertices, v2, v0);
            
            newIndices.push_back(v0); newIndices.push_back(a); newIndices.push_back(c);
            newIndices.push_back(v1); newIndices.push_back(b); newIndices.push_back(a);
            newIndices.push_back(v2); newIndices.push_back(c); newIndices.push_back(b);
            newIndices.push_back(a); newIndices.push_back(b); newIndices.push_back(c);
            
            subdivided = true;
        } else if (d0 > threshold) {
            // Only the edge v0-v1 is longer than the threshold
            uint16_t a = findOrCreateMidpoint(midpointCache, vertices, v0, v1);
            
            newIndices.push_back(v0); newIndices.push_back(a); newIndices.push_back(v2);
            newIndices.push_back(a); newIndices.push_back(v1); newIndices.push_back(v2);
            
            subdivided = true;
        } else if (d1 > threshold) {
            // Only the edge v1-v2 is longer than the threshold
            uint16_t b = findOrCreateMidpoint(midpointCache, vertices, v1, v2);
            
            newIndices.push_back(v1); newIndices.push_back(b); newIndices.push_back(v0);
            newIndices.push_back(b); newIndices.push_back(v2); newIndices.push_back(v0);
            
            subdivided = true;
        } else if (d2 > threshold) {
            // Only the edge v2-v0 is longer than the threshold
            uint16_t c = findOrCreateMidpoint(midpointCache, vertices, v2, v0);
            
            newIndices.push_back(v2); newIndices.push_back(c); newIndices.push_back(v1);
            newIndices.push_back(c); newIndices.push_back(v0); newIndices.push_back(v1);
            
            subdivided = true;
        } else {
            // No edges are longer than the threshold, keep original triangle
            newIndices.push_back(v0);
            newIndices.push_back(v1);
            newIndices.push_back(v2);
        }
    }
    
    // Replace old indices with new indices
    indices = std::move(newIndices);
    
    // Recursively subdivide if any triangles were subdivided
    if (subdivided) {
        subdivision(vertices, indices, threshold, level - 1);
    }
}

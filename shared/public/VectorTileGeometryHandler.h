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
#include "GeoJsonTypes.h"
#include "EarcutVectorView.h"

namespace mapbox {
    namespace util {
        template <>
        struct nth<0, vtzero::point> {
            inline static auto get(const vtzero::point &t) {
                return t.x;
            };
        };
        template <>
        struct nth<1, vtzero::point> {
            inline static auto get(const vtzero::point &t) {
                return t.y;
            };
        };

        template <>
        struct nth<0, Coord> {
            inline static auto get(const Coord &t) {
                return t.x;
            };
        };
        template <>
        struct nth<1, Coord> {
            inline static auto get(const Coord &t) {
                return t.y;
            };
        };
    } // namespace util
} // namespace mapbox


class VectorTileGeometryHandler {
public:
    VectorTileGeometryHandler(::RectCoord tileCoords, int extent, const std::optional<Tiled2dMapVectorSettings> &vectorSettings, const std::shared_ptr<CoordinateConversionHelperInterface> &conversionHelper)
    : tileCoords(tileCoords),
      // use standard TOP_LEFT origin, when no vector settings given.
      origin(vectorSettings ? vectorSettings->tileOrigin : Tiled2dMapVectorTileOrigin::TOP_LEFT),
      extent((double)extent),
      conversionHelper(conversionHelper)
    {};

    VectorTileGeometryHandler(const std::shared_ptr<GeoJsonGeometry> &geometry, ::RectCoord tileCoords, const std::shared_ptr<CoordinateConversionHelperInterface> &conversionHelper): tileCoords(tileCoords), conversionHelper(conversionHelper) {
        switch (geometry->featureContext->geomType) {
            case vtzero::GeomType::POINT:
            case vtzero::GeomType::LINESTRING: {
                for (auto const &points: geometry->coordinates) {
                    std::vector<Vec2D> temp;
                    for (auto const &point: points) {
                        //TODO: change geojson to vec2d as well
                        temp.push_back(Vec2D(point.x, point.y));
                    }
                    coordinates.push_back(temp);
                }
                break;
            }
            case vtzero::GeomType::POLYGON:
                triangulateGeoJsonPolygons(geometry);
                break;
            case vtzero::GeomType::UNKNOWN:
                break;
        }
    }

    VectorTileGeometryHandler(const VectorTileGeometryHandler& other) = delete;

    void points_begin(const uint32_t count) {
        coordinates.emplace_back();
        coordinates.back().reserve(count);
    }

    void points_point(const vtzero::point point) {
        coordinates.back().emplace_back(coordinateFromPoint(point, false));
    }

    void points_end() {
        // all done.
    }

    void linestring_begin(const uint32_t count) {
        coordinates.emplace_back();
        coordinates.back().reserve(count);
    }

    void linestring_point(const vtzero::point point) {
        coordinates.back().emplace_back(coordinateFromPoint(point, false));
    }

    void linestring_end() {
    }

    void ring_begin(uint32_t count) {
        polygonRings.emplace_back();
        polygonRings.back().points.reserve(count);
    }

    void ring_point(vtzero::point point) noexcept {
        polygonRings.back().points.emplace_back(point);
    }

    void ring_end(vtzero::ring_type ringType) noexcept {
        if (polygonRings.empty()) { return; }

        auto& ring = polygonRings.back();
        if (ring.points.empty()) {
            polygonRings.pop_back(); // Remove empty ring
            return;
        }

        ring.points.push_back(ring.points[0]);

        switch (ringType) {
            case vtzero::ring_type::outer:
                ring.isHole = false;
                break;
            case vtzero::ring_type::inner:
                ring.isHole = true;
                break;
            case vtzero::ring_type::invalid:
                polygonRings.pop_back(); // Discard invalid ring
                break;
        }
    }

    size_t beginTriangulatePolygons() {
        computePolygonRanges();
        
        return polygonRanges.size();
    }

    void endTringulatePolygons() {
        polygonRings.clear();
        polygonRanges.clear();
    }

    void triangulatePolygons(size_t i, mapbox::detail::Earcut<uint16_t> &earcutter) {
        const auto& range = polygonRanges[i];

        coordinates.reserve(coordinates.size() + range.size);

        for(size_t i = range.startIndex; i < range.size + range.startIndex; ++i) {
            coordinates.emplace_back();
            coordinates.back().reserve(polygonRings[i].points.size());

            for (auto const &point: polygonRings[i].points){
                coordinates.back().push_back(coordinateFromPoint(point, false));
            }
        }

        auto polygonView = EarcutPolygonVectorView(polygonRings, polygonRanges[i], 500);
        earcutter(polygonView);
        std::vector<uint16_t> indices = std::move(earcutter.indices);

        std::reverse(indices.begin(), indices.end());

        size_t numCoordinates = 0;
        for(size_t i=0; i<polygonView.size(); ++i) {
            numCoordinates += polygonView[i].size();
        }
        polygons.push_back({{}, indices});
        auto &coordinates = polygons.back().coordinates;
        coordinates.reserve(numCoordinates);
        for(size_t i=0; i<polygonView.size(); ++i) {
            for(auto const &point : polygonView[i]) {
                coordinates.push_back(vecFromPoint(point));
            }
        }
    }

    void triangulateGeoJsonPolygons(const std::shared_ptr<GeoJsonGeometry> &geometry) {
        mapbox::detail::Earcut<uint16_t> earcutter;
        for (int i = 0; i < geometry->coordinates.size(); i++) {
            coordinates.reserve(coordinates.size() + geometry->coordinates[i].size() + 1);

            coordinates.emplace_back();
            coordinates.back().reserve(geometry->coordinates[i].size());
            for (auto const &point: geometry->coordinates[i]){
                coordinates.back().push_back(Vec2D(point.x, point.y));
            }

            for(auto const &hole: geometry->holes[i]) {
                coordinates.emplace_back();
                coordinates.back().reserve(hole.size());
                for (auto const &point: hole){
                    coordinates.back().push_back(Vec2D(point.x, point.y));
                }
            }

            auto polygonView = EarcutCoordVectorView(geometry->coordinates[i], geometry->holes[i], 500);
            earcutter(polygonView);
            std::vector<uint16_t> indices = std::move(earcutter.indices);

            if (!indices.empty()) {

                size_t numCoordinates = 0;
                for(size_t i=0; i<polygonView.size(); ++i) {
                    numCoordinates += polygonView[i].size();
                }
                polygons.push_back({{}, indices});

                auto &coordinates = polygons.back().coordinates;
                coordinates.reserve(numCoordinates);
                for(size_t i=0; i<polygonView.size(); ++i) {
                    for(auto const &point : polygonView[i]) {
                        const auto &converted = conversionHelper->convertToRenderSystem(point);
                        coordinates.push_back(Vec2D(converted.x, converted.y));
                    }
                }
            }
        }
    }

    std::vector<std::vector<::Vec2D>> &getLineCoordinates() {
        return coordinates;
    }

    struct TriangulatedPolygon {
        std::vector<Vec2D> coordinates;
        std::vector<uint16_t> indices;
    };

    const std::vector<TriangulatedPolygon> &getPolygons() const {
        return polygons;
    }

    const std::vector<std::vector<::Vec2D>> &getPointCoordinates() const {
        return coordinates;
    }

    static inline double signedTriangleArea(const Vec2D& a, const Vec2D& b, const Vec2D& c) {
        return (b.x - a.x) * (c.y - a.y) - (c.x - a.x) * (b.y - a.y);
    }

    // Check if a point is inside a polygon using the winding number algorithm
    static bool isPointInTriangulatedPolygon(const Coord& point, const TriangulatedPolygon& polygon, const std::shared_ptr<CoordinateConversionHelperInterface> &conversionHelper) {
        if (polygon.coordinates.empty()) {
            return false;
        }
        const auto convertedPoint = conversionHelper->convertToRenderSystem(point);
        const auto vecPoint = Vec2D(convertedPoint.x, convertedPoint.y);

        float d1, d2, d3;
        bool has_neg, has_pos;

        for (size_t i = 0; i < polygon.indices.size(); i += 3) {
            const auto& v1 = polygon.coordinates[polygon.indices[i]];
            const auto& v2 = polygon.coordinates[polygon.indices[i + 1]];
            const auto& v3 = polygon.coordinates[polygon.indices[i + 2]];

            d1 = signedTriangleArea(v1, v2, vecPoint);
            d2 = signedTriangleArea(v2, v3, vecPoint);
            d3 = signedTriangleArea(v3, v1, vecPoint);

            has_neg = (d1 < 0) || (d2 < 0) || (d3 < 0);
            has_pos = (d1 > 0) || (d2 > 0) || (d3 > 0);

            if (!(has_neg && has_pos)) {
                return true;
            }
        }
        return false;
    }

private:
    inline Vec2D coordinateFromPoint(const vtzero::point &point, bool renderSystem) {
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

        const auto x = tileCoords.topLeft.x * (1.0 - tx) + tileCoords.bottomRight.x * tx;
        const auto y = tileCoords.topLeft.y * (1.0 - ty) + tileCoords.bottomRight.y * ty;

        if (renderSystem) {
            const auto coord = conversionHelper->convertToRenderSystem(Coord(tileCoords.topLeft.systemIdentifier, x, y, 0.0));
            return Vec2D(coord.x, coord.y);
        } else {
            return Vec2D(x, y);
        }
    }

    inline Vec2D vecFromPoint(const vtzero::point &point) {
        return coordinateFromPoint(point, true);
    }

    void computePolygonRanges() {
        size_t start = 0;
        size_t count = 0;

        for (size_t i = 0; i < polygonRings.size(); ++i) {
            const PolygonRing& ring = polygonRings[i];

            if (!ring.isHole) {
                // Start of a new polygon
                if (count > 0) {
                    polygonRanges.push_back({start, count});
                }
                start = i;
                count = 1;
            } else {
                ++count;
            }
        }

        // Final polygon
        if (count > 0) {
            polygonRanges.push_back({start, count});
        }
    }

    std::vector<std::vector<::Vec2D>> coordinates;

    std::vector<PolygonRing> polygonRings;
    std::vector<PolygonRange> polygonRanges;

    std::vector<TriangulatedPolygon> polygons;

    Tiled2dMapVectorTileOrigin origin;
    RectCoord tileCoords;
    double extent;

    const std::shared_ptr<CoordinateConversionHelperInterface> conversionHelper;
};

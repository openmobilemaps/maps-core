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
                    std::vector<Coord> temp;
                    for (auto const &point: points) {
                        temp.push_back(point);
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
        polygonCurrentRing = std::vector<vtzero::point>();
        polygonCurrentRing.reserve(count);
    }

    void ring_point(vtzero::point point) noexcept {
        polygonCurrentRing.emplace_back(point);
    }

    void ring_end(vtzero::ring_type ringType) noexcept {
        if (!polygonCurrentRing.empty()) {
            polygonCurrentRing.push_back(polygonCurrentRing[0]);

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

    void triangulatePolygons() {
        if (polygonPoints.empty()) {
            return;
        }

        for (int i = 0; i < polygonPoints.size(); i++) {
            std::vector<std::vector<vtzero::point>> polygon;
            polygon.reserve(polygonHoles[i].size() + 1);
            coordinates.reserve(coordinates.size() + polygon.capacity());

            coordinates.emplace_back();
            coordinates.back().reserve(polygonPoints[i].size());
            for (auto const &point: polygonPoints[i]){
                coordinates.back().push_back(coordinateFromPoint(point, false));
            }
            polygon.emplace_back(std::move(polygonPoints[i]));

            for(auto const &hole: polygonHoles[i]) {
                coordinates.emplace_back();
                coordinates.back().reserve(hole.size());
                for (auto const &point: hole){
                    coordinates.back().push_back(coordinateFromPoint(point, false));
                }

                polygon.emplace_back(std::move(hole));
            }
            limitHoles(polygon, 500);

            std::vector<uint16_t> indices = mapbox::earcut<uint16_t>(polygon);

            std::reverse(indices.begin(), indices.end());

            polygons.push_back({{}, indices});

            for (auto const &points: polygon) {
                for (auto const &point: points) {
                    polygons.back().coordinates.push_back(vecFromPoint(point));
                }
            }
        }

        polygonHoles.clear();
        polygonPoints.clear();
    }

    void triangulateGeoJsonPolygons(const std::shared_ptr<GeoJsonGeometry> &geometry) {
        for (int i = 0; i < geometry->coordinates.size(); i++) {
            std::vector<std::vector<Coord>> polygon;
            polygon.reserve(geometry->coordinates[i].size() + 1);
            coordinates.reserve(coordinates.size() + polygon.capacity());

            coordinates.emplace_back();
            coordinates.back().reserve(geometry->coordinates[i].size());
            for (auto const &point: geometry->coordinates[i]){
                coordinates.back().push_back(point);
            }
            polygon.emplace_back(geometry->coordinates[i]);

            for(auto const &hole: geometry->holes[i]) {
                coordinates.emplace_back();
                coordinates.back().reserve(hole.size());
                for (auto const &point: hole){
                    coordinates.back().push_back(point);
                }

                polygon.emplace_back(hole);
            }
            limitHoles(polygon, 500);

            std::vector<uint16_t> indices = mapbox::earcut<uint16_t>(polygon);

            if (!indices.empty()) {
                polygons.push_back({{}, indices});

                for (auto const &points: polygon) {
                    for (auto const &point: points) {
                        auto converted = conversionHelper->convertToRenderSystem(point);
                        polygons.back().coordinates.push_back(Vec2F(converted.x, converted.y));
                    }
                }
            }
        }
    }

    std::vector<std::vector<::Coord>> &getLineCoordinates() {
        return coordinates;
    }

    struct TriangulatedPolygon {
        std::vector<Vec2F> coordinates;
        std::vector<uint16_t> indices;
    };

    const std::vector<TriangulatedPolygon> &getPolygons() const {
        return polygons;
    }

    const std::vector<std::vector<::Coord>> &getPointCoordinates() const {
        return coordinates;
    }

    static inline double signedTriangleArea(const Vec2F& a, const Vec2F& b, const Vec2F& c) {
        return (b.x - a.x) * (c.y - a.y) - (c.x - a.x) * (b.y - a.y);
    }

    // Check if a point is inside a polygon using the winding number algorithm
    static bool isPointInTriangulatedPolygon(const Coord& point, const TriangulatedPolygon& polygon, const std::shared_ptr<CoordinateConversionHelperInterface> &conversionHelper) {
        if (polygon.coordinates.empty()) {
            return false;
        }
        const auto convertedPoint = conversionHelper->convertToRenderSystem(point);
        const auto vecPoint = Vec2F(convertedPoint.x, convertedPoint.y);

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
    inline Coord coordinateFromPoint(const vtzero::point &point, bool renderSystem) {
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
            return conversionHelper->convertToRenderSystem(Coord(tileCoords.topLeft.systemIdentifier, x, y, 0.0));
        } else {
            return Coord(tileCoords.topLeft.systemIdentifier, x, y, 0.0);
        }
    }

    inline Vec2F vecFromPoint(const vtzero::point &point) {
        const auto coord = coordinateFromPoint(point, true);
        return Vec2F(coord.x, coord.y);
    }


    static double signedArea(const std::vector<vtzero::point>& hole) {
        double sum = 0;
        for (std::size_t i = 0, len = hole.size(), j = len - 1; i < len; j = i++) {
            const auto& p1 = hole[i];
            const auto& p2 = hole[j];
            sum += (p2.x - p1.x) * (p1.y + p2.y);
        }
        return sum;
    }

    void limitHoles(std::vector<std::vector<vtzero::point>> &polygon, uint32_t maxHoles) {
        if (polygon.size() > 1 + maxHoles) {
             std::nth_element(
                 polygon.begin() + 1, polygon.begin() + 1 + maxHoles, polygon.end(), [](const auto& a, const auto& b) {
                     return std::fabs(signedArea(a)) > std::fabs(signedArea(b));
                 });
             polygon.resize(1 + maxHoles);
         }
    }

    static double signedArea(const std::vector<Coord>& hole) {
        double sum = 0;
        for (std::size_t i = 0, len = hole.size(), j = len - 1; i < len; j = i++) {
            const auto& p1 = hole[i];
            const auto& p2 = hole[j];
            sum += (p2.x - p1.x) * (p1.y + p2.y);
        }
        return sum;
    }

    void limitHoles(std::vector<std::vector<Coord>> &polygon, uint32_t maxHoles) {
        if (polygon.size() > 1 + maxHoles) {
             std::nth_element(
                 polygon.begin() + 1, polygon.begin() + 1 + maxHoles, polygon.end(), [](const auto& a, const auto& b) {
                     return std::fabs(signedArea(a)) > std::fabs(signedArea(b));
                 });
             polygon.resize(1 + maxHoles);
         }
    }

    std::vector<::Coord> currentFeature;
    std::vector<std::vector<::Coord>> coordinates;

    std::vector<vtzero::point> polygonCurrentRing;
    std::vector<std::vector<vtzero::point>> polygonPoints;
    std::vector<std::vector<std::vector<vtzero::point>>> polygonHoles;

    std::vector<TriangulatedPolygon> polygons;

    Tiled2dMapVectorTileOrigin origin;
    RectCoord tileCoords;
    double extent;

    const std::shared_ptr<CoordinateConversionHelperInterface> conversionHelper;
};

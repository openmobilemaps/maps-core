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
#include "Value.h"
#include "Vec2D.h"
#include <vector>
#include <limits>

class GeoJsonGeometry {
public:
    GeoJsonGeometry(){}
    GeoJsonGeometry(const std::shared_ptr<FeatureContext> featureContext,
                    const std::vector<std::vector<::Coord>> coordinates,
                    const std::vector<std::vector<std::vector<::Coord>>> holes): featureContext(featureContext), coordinates(coordinates), holes(holes) {
        updateMinMax();
    }
    std::shared_ptr<FeatureContext> featureContext;
    std::vector<std::vector<::Coord>> coordinates;
    std::vector<std::vector<std::vector<::Coord>>> holes;

    Vec2D bboxMin = Vec2D(std::numeric_limits<float>::max(), std::numeric_limits<float>::max());
    Vec2D bboxMax = Vec2D(std::numeric_limits<float>::lowest(), std::numeric_limits<float>::lowest());

private:
    void updateMinMax() {
        for (const auto& points : coordinates) {
            for (const auto& point : points) {
                bboxMin.x = std::min(point.x, bboxMin.x);
                bboxMin.y = std::min(point.y, bboxMin.y);
                bboxMax.x = std::max(point.x, bboxMax.x);
                bboxMax.y = std::max(point.y, bboxMax.y);
            }
        }

        for (const auto& hole : holes) {
            for (const auto& points : hole) {
                for (const auto& point : points) {
                    bboxMin.x = std::min(point.x, bboxMin.x);
                    bboxMin.y = std::min(point.y, bboxMin.y);
                    bboxMax.x = std::max(point.x, bboxMax.x);
                    bboxMax.y = std::max(point.y, bboxMax.y);
                }
            }
        }
    }
};

template <uint8_t I, typename T>
inline double get(const T&);

template <>
inline double get<0>(const Coord& p) {
    return p.x;
}

template <>
inline double get<1>(const Coord& p) {
    return p.y;
}

template <>
inline double get<0>(const Vec2D& p) {
    return p.x;
}
template <>
inline double get<1>(const Vec2D& p) {
    return p.y;
}

template <uint8_t I>
inline double calc_progress(const Coord&, const Coord&, const double);

template <>
inline double calc_progress<0>(const Coord& a, const Coord& b, const double x) {
    return (x - a.x) / (b.x - a.x);
}

template <>
inline double calc_progress<1>(const Coord& a, const Coord& b, const double y) {
    return (y - a.y) / (b.y - a.y);
}

template <uint8_t I>
inline Coord intersect(const Coord&, const Coord&, const double, const double);

template <>
inline Coord intersect<0>(const Coord& a, const Coord& b, const double x, const double t) {
    const double y = (b.y - a.y) * t + a.y;
    return { a.systemIdentifier, x, y, 1.0 };
}
template <>
inline Coord intersect<1>(const Coord& a, const Coord& b, const double y, const double t) {
    const double x = (b.x - a.x) * t + a.x;
    return { a.systemIdentifier, x, y, 1.0 };
}

class GeoJson {
public:
    std::vector<std::shared_ptr<GeoJsonGeometry>> geometries;
};


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
#include "DataLoaderResult.h"
#include "Future.hpp"
#include "Actor.h"
#include "LoaderInterface.h"

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

class GeoJSONTileInterface {
public:
    virtual const std::vector<std::shared_ptr<GeoJsonGeometry>>& getFeatures() const = 0;
};

class GeoJSONTileDelegate {
public:
    virtual void didLoad(uint8_t maxZoom) = 0;
    virtual void failedToLoad() = 0;
};

class GeoJson {
public:
    std::vector<std::shared_ptr<GeoJsonGeometry>> geometries;
    bool hasOnlyPoints = true;
};

class GeoJSONVTInterface {
public:
    virtual const GeoJSONTileInterface& getTile(const uint8_t z, const uint32_t x_, const uint32_t y) = 0;
    virtual bool isLoaded() = 0;
    virtual void waitIfNotLoaded(std::shared_ptr<::djinni::Promise<std::shared_ptr<DataLoaderResult>>> promise) = 0;
    virtual uint8_t getMinZoom() = 0;
    virtual uint8_t getMaxZoom() = 0;
    virtual void reload(const std::vector<std::shared_ptr<::LoaderInterface>> &loaders) = 0;
    virtual void reload(const std::shared_ptr<GeoJson> &geoJson) = 0;
    virtual void setDelegate(const WeakActor<GeoJSONTileDelegate> delegate) = 0;
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




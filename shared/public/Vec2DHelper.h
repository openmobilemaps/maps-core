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

#include "Vec2D.h"
#include "Quad2dD.h"
#include <algorithm>
#include <vector>
#include <cmath>


inline Vec2D operator+( const ::Vec2D& left, const ::Vec2D& right ) {
    return Vec2D(left.x + right.x, left.y + right.y);
}

inline Vec2D operator-( const ::Vec2D& left, const ::Vec2D& right ) {
    return Vec2D(left.x - right.x, left.y - right.y);
}

inline double operator*( const ::Vec2D& left, const ::Vec2D& right ) {
    return left.x * right.x + left.y * right.y;
}

inline Vec2D operator*( const ::Vec2D& left, const double& val) {
    return Vec2D(left.x * val, left.y * val);
}

inline Vec2D operator*( const ::Vec2D& left, const float& val) {
    return Vec2D(left.x * val, left.y * val);
}

inline Vec2D operator/( const ::Vec2D& left, const double& val ) {
    return Vec2D(left.x / val, left.y / val);
}

class Vec2DHelper {
  public:
    static inline double distance(const ::Vec2D &from, const ::Vec2D &to) {
        return std::sqrt((from.x - to.x) * (from.x - to.x) + (from.y - to.y) * (from.y - to.y));
    }

    static inline double distanceSquared(const ::Vec2D &from, const ::Vec2D &to) {
        return (from.x - to.x) * (from.x - to.x) + (from.y - to.y) * (from.y - to.y);
    }

    static inline ::Vec2D midpoint(const ::Vec2D &from, const ::Vec2D &to) {
        return Vec2D((from.x + to.x) / 2.0, (from.y + to.y) / 2.0);
    }

    static inline double crossProduct(const Vec2D& A, const Vec2D& B, const Vec2D& C) {
        return (B.x - A.x) * (C.y - A.y) - (B.y - A.y) * (C.x - A.x);
    }

    static inline double crossProduct(const Vec2D& A, const Vec2D& B) {
        return (A.x * B.y) - (A.y * B.x);
    }

    static inline ::Vec2D rotate(const ::Vec2D &p, const ::Vec2D &origin, double angleDegree) {
        const double sinAngle = sin(angleDegree * M_PI / 180.0);
        const double cosAngle = cos(angleDegree * M_PI / 180.0);
        const double x = p.x - origin.x;
        const double y = p.y - origin.y;
        const double rX = x * cosAngle - y * sinAngle;
        const double rY = x * sinAngle + y * cosAngle;
        return Vec2D(rX + origin.x, rY + origin.y);
    }

    // returns the indices that form the convex hull
    static inline std::vector<size_t> convexHull(std::vector<Vec2D>& points) {
        std::sort(points.begin(), points.end());

        std::vector<size_t> hull;

        if (points.size() <= 3) {
            for (size_t i = 0; i != points.size(); i++) {
                hull.push_back(i);
            }
            return hull;
        }


        hull.push_back(0);
        hull.push_back(1);

        for (size_t i = 2; i < points.size(); ++i) {
            while (hull.size() >= 2) {
                size_t lastIndex = hull.size() - 1;
                double cross = Vec2DHelper::crossProduct(points[hull[lastIndex - 1]], points[hull[lastIndex]], points[i]);
                if (cross <= 0) {
                    hull.pop_back();
                } else {
                    break;
                }
            }
            hull.push_back(i);
        }

        return hull;
    }

    static inline Quad2dD minimumAreaEnclosingRectangle(std::vector<Vec2D>& points) {
        const std::vector<size_t> &hull = Vec2DHelper::convexHull(points);

        double minArea = std::numeric_limits<double>::max();
        Quad2dD minRectangle({0, 0}, {0, 0}, {0, 0}, {0, 0});

        for (size_t i = 0; i < hull.size(); ++i) {
            const size_t nextIndex = (i + 1) % hull.size();
            const auto &thisIndexPoint = points[hull[i]];
            const auto &nextIndexPoint = points[hull[nextIndex]];
            const double dx = nextIndexPoint.x - thisIndexPoint.x;
            const double dy = nextIndexPoint.y - thisIndexPoint.y;
            const double angle = std::atan2(dy, dx);

            double minX = std::numeric_limits<double>::max();
            double maxX = std::numeric_limits<double>::lowest();
            double minY = std::numeric_limits<double>::max();
            double maxY = std::numeric_limits<double>::lowest();
            for (const Vec2D& point : points) {
                const double rotatedX = (point.x - thisIndexPoint.x) * std::cos(-angle) - (point.y - thisIndexPoint.y) * std::sin(-angle);
                const double rotatedY = (point.x - thisIndexPoint.x) * std::sin(-angle) + (point.y - thisIndexPoint.y) * std::cos(-angle);
                minX = std::min(minX, rotatedX);
                maxX = std::max(maxX, rotatedX);
                minY = std::min(minY, rotatedY);
                maxY = std::max(maxY, rotatedY);
            }

            const double width = maxX - minX;
            const double height = maxY - minY;
            const double area = width * height;

            if (area < minArea) {
                minArea = area;
                minRectangle.topLeft.x = thisIndexPoint.x + minX * std::cos(angle) - minY * std::sin(angle);
                minRectangle.topLeft.y = thisIndexPoint.y + minX * std::sin(angle) + minY * std::cos(angle);
                minRectangle.topRight.x = thisIndexPoint.x + maxX * std::cos(angle) - minY * std::sin(angle);
                minRectangle.topRight.y = thisIndexPoint.y + maxX * std::sin(angle) + minY * std::cos(angle);
                minRectangle.bottomRight.x = thisIndexPoint.x + maxX * std::cos(angle) - maxY * std::sin(angle);
                minRectangle.bottomRight.y = thisIndexPoint.y + maxX * std::sin(angle) + maxY * std::cos(angle);
                minRectangle.bottomLeft.x = thisIndexPoint.x + minX * std::cos(angle) - maxY * std::sin(angle);
                minRectangle.bottomLeft.y = thisIndexPoint.y + minX * std::sin(angle) + maxY * std::cos(angle);
            }
        }

        return minRectangle;
    }

    static inline double squaredLength(const ::Vec2D &vector) {
        return vector * vector;
    }

    static inline ::Vec2D normalize(const ::Vec2D &vector) {
        return vector / std::sqrt(Vec2DHelper::squaredLength(vector));
    }
};

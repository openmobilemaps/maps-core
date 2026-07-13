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
#include "Vec2D.h"
#include "Vec3D.h"
#include "Quad2dD.h"
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

inline Vec2D& operator*=(Vec2D& left, const double& val) {
    left.x *= val;
    left.y *= val;
    return left;
}

inline Vec2D& operator/=(Vec2D& left, const double& val) {
    left.x /= val;
    left.y /= val;
    return left;
}

inline Vec2D operator-(const Vec2D& vec) {
    return Vec2D(-vec.x, -vec.y);
}

class Vec2DHelper {
  public:
    static inline double distance(const ::Vec2D &from, const ::Vec2D &to) {
        return std::sqrt(distanceSquared(from, to));
    }

    static inline double distanceSquared(const ::Vec2D &from, const ::Vec2D &to) {
        return (from.x - to.x) * (from.x - to.x) + (from.y - to.y) * (from.y - to.y);
    }
    
    static inline double length(const ::Vec2D &vector) {
        return std::sqrt(vector * vector);
    }
    
    static inline double lengthSquared(const ::Vec2D &vector) {
        return vector * vector;
    }

    static inline ::Vec2D normalize(const ::Vec2D &vector) {
        return vector / length(vector);
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

    static inline Vec2D interpolate(const Vec2D& start, const Vec2D& end, double t) {
        return Vec2D(start.x + (end.x - start.x) * t,
                     start.y + (end.y - start.y) * t);
    }

    static inline ::Vec2D rotate(const ::Vec2D &p, const ::Vec2D &origin, double angleDegree) {
        const double sinAngle = std::sin(angleDegree * M_PI / 180.0);
        const double cosAngle = std::cos(angleDegree * M_PI / 180.0);
        return rotate(p, origin, sinAngle, cosAngle);
    }

    static inline ::Vec2D rotate(const ::Vec2D &p, const ::Vec2D &origin, double sinAngle, double cosAngle) {
        const double x = p.x - origin.x;
        const double y = p.y - origin.y;
        const double rX = x * cosAngle - y * sinAngle;
        const double rY = x * sinAngle + y * cosAngle;
        return Vec2D(rX + origin.x, rY + origin.y);
    }

    // returns the indices that form the convex hull
    static std::vector<size_t> convexHull(std::vector<Vec2D>& points);

    static inline Quad2dD minimumAreaEnclosingRectangle(std::vector<Vec2D>& points);

    static inline ::Vec2D toVec(const ::Coord &coordinate) {
        return Vec2D(coordinate.x, coordinate.y);
    }

    static inline ::Vec3D toVec3D(const ::Vec2D &vec) {
        return Vec3D(vec.x, vec.y, 0.0);
    }

    static inline ::Coord toCoord(const ::Vec2D &vec, const int32_t systemIdentifier) {
        return Coord(systemIdentifier, vec.x, vec.y, 0.0);
    }
};

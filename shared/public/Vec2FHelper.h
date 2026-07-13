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
#include "Vec2F.h"

#include <cmath>

inline Vec2F operator+( const Vec2F& left, const Vec2F& right ) {
    return Vec2F(left.x + right.x, left.y + right.y);
}

inline Vec2F operator-( const Vec2F& left, const Vec2F& right ) {
    return Vec2F(left.x - right.x, left.y - right.y);
}

inline float operator*( const Vec2F& left, const Vec2F& right ) {
    return left.x * right.x + left.y * right.y;
}

inline Vec2F operator*( const Vec2F& left, const float& val ) {
    return Vec2F(left.x * val, left.y * val);
}

inline Vec2F operator/( const Vec2F& left, const float& val ) {
    return Vec2F(left.x / val, left.y / val);
}

inline Vec2F& operator*=(Vec2F& left, const float& val) {
    left.x *= val;
    left.y *= val;
    return left;
}

inline Vec2F& operator/=(Vec2F& left, const float& val) {
    left.x /= val;
    left.y /= val;
    return left;
}

inline Vec2F operator-(const Vec2F& vec) {
    return Vec2F(-vec.x, -vec.y);
}

class Vec2FHelper {
  public:
    static float distance(const Vec2F &from, const Vec2F &to) {
        return std::sqrt(distanceSquared(from, to));
    }
    
    static inline float distanceSquared(const Vec2F &from, const Vec2F &to) {
        return (from.x - to.x) * (from.x - to.x) + (from.y - to.y) * (from.y - to.y);
    }

    static float length(const Vec2F &vector) {
        return std::sqrt(vector * vector);
    }

    static float lengthSquared(const Vec2F &vector) {
        return vector * vector;
    }
    
    static Vec2F normalize(const Vec2F &vector) {
        return vector / length(vector);
    }

    static Vec2F midpoint(const Vec2F &from, const Vec2F &to) {
        return Vec2F((from.x + to.x) / 2.0, (from.y + to.y) / 2.0);
    }

    static Vec2F rotate(const Vec2F &p, const Vec2F &origin, float angleDegree) {
        float sinAngle = std::sin(angleDegree * M_PI / 180.0);
        float cosAngle = std::cos(angleDegree * M_PI / 180.0);
        return rotate(p, origin, sinAngle, cosAngle);
    }

    static Vec2F rotate(const Vec2F &p, const Vec2F &origin, float sinAngle, float cosAngle) {
        float x = p.x - origin.x;
        float y = p.y - origin.y;
        float rX = x * cosAngle - y * sinAngle;
        float rY = x * sinAngle + y * cosAngle;
        return Vec2F(rX + origin.x, rY + origin.y);
    }
};

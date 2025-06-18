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
#include "Vec3D.h"
#include "Quad2dD.h"
#include <vector>
#include <cmath>


inline Vec3D operator+( const ::Vec3D& left, const ::Vec3D& right ) {
    return Vec3D(left.x + right.x, left.y + right.y, left.z + right.z);
}

inline Vec3D operator-( const ::Vec3D& left, const ::Vec3D& right ) {
    return Vec3D(left.x - right.x, left.y - right.y, left.z - right.z);
}

inline double operator*( const ::Vec3D& left, const ::Vec3D& right ) {
    return left.x * right.x + left.y * right.y + left.z * right.z;
}

inline Vec3D operator*( const ::Vec3D& left, const double& val) {
    return Vec3D(left.x * val, left.y * val, left.z * val);
}

inline Vec3D operator/( const ::Vec3D& left, const double& val ) {
    return Vec3D(left.x / val, left.y / val, left.z / val);
}

class Vec3DHelper {
  public:
    static inline double distance(const ::Vec3D &from, const ::Vec3D &to) {
        return std::sqrt((from.x - to.x) * (from.x - to.x) + (from.y - to.y) * (from.y - to.y) + (from.z - to.z) * (from.z - to.z));
    }

    static inline double distanceSquared(const ::Vec3D &from, const ::Vec3D &to) {
        return (from.x - to.x) * (from.x - to.x) + (from.y - to.y) * (from.y - to.y) + (from.z - to.z) * (from.z - to.z);
    }

    static inline ::Vec3D midpoint(const ::Vec3D &from, const ::Vec3D &to) {
        return Vec3D((from.x + to.x) / 2.0, (from.y + to.y) / 2.0, (from.z + to.z) / 2.0);
    }

    static inline double squaredLength(const ::Vec3D &vector) {
        return vector * vector;
    }

    static inline ::Vec3D normalize(const ::Vec3D &vector) {
        return vector / std::sqrt(Vec3DHelper::squaredLength(vector));
    }

    static inline ::Vec3D toVec(const ::Coord &coordinate) {
        return Vec3D(coordinate.x, coordinate.y, coordinate.z);
    }

    static inline ::Coord toCoord(const ::Vec3D &vec, const int32_t systemIdentifier) {
        return Coord(systemIdentifier, vec.x, vec.y, vec.z);
    }
};

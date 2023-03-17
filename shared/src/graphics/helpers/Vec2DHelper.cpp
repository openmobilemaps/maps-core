/*
 * Copyright (c) 2021 Ubique Innovation AG <https://www.ubique.ch>
 *
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this
 *  file, You can obtain one at https://mozilla.org/MPL/2.0/.
 *
 *  SPDX-License-Identifier: MPL-2.0
 */

#include "Vec2DHelper.h"
#include <cmath>

double Vec2DHelper::distance(const ::Vec2D &from, const ::Vec2D &to) {
    return std::sqrt((from.x - to.x) * (from.x - to.x) + (from.y - to.y) * (from.y - to.y));
}

double Vec2DHelper::distanceSquared(const ::Vec2D &from, const ::Vec2D &to) {
    return (from.x - to.x) * (from.x - to.x) + (from.y - to.y) * (from.y - to.y);
}

::Vec2D Vec2DHelper::midpoint(const ::Vec2D &from, const ::Vec2D &to) {
    return Vec2D((from.x + to.x) / 2.0, (from.y + to.y) / 2.0);
}

::Vec2D Vec2DHelper::rotate(const Vec2D &p, const Vec2D &origin, double angleDegree) {
    double sinAngle = sin(angleDegree * M_PI / 180.0);
    double cosAngle = cos(angleDegree * M_PI / 180.0);
    double x = p.x - origin.x;
    double y = p.y - origin.y;
    double rX = x * cosAngle - y * sinAngle;
    double rY = x * sinAngle + y * cosAngle;
    return Vec2D(rX + origin.x, rY + origin.y);
}

double Vec2DHelper::squaredLength(const ::Vec2D &vector) {
    return vector * vector;
}

Vec2D operator+( const ::Vec2D& left, const ::Vec2D& right ) {
    return Vec2D(left.x + right.x, left.y + right.y);
}

Vec2D operator-( const ::Vec2D& left, const ::Vec2D& right ) {
    return Vec2D(left.x - right.x, left.y - right.y);
}

double operator*( const ::Vec2D& left, const ::Vec2D& right ) {
    return left.x * right.x + left.y * right.y;
}

Vec2D operator/( const ::Vec2D& left, const double& val ) {
    return Vec2D(left.x / val, left.y / val);
}

Vec2D Vec2DHelper::normalize(const ::Vec2D &vector) {
    return vector / std::sqrt(Vec2DHelper::squaredLength(vector));
}

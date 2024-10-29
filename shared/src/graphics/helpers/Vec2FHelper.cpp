/*
 * Copyright (c) 2021 Ubique Innovation AG <https://www.ubique.ch>
 *
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this
 *  file, You can obtain one at https://mozilla.org/MPL/2.0/.
 *
 *  SPDX-License-Identifier: MPL-2.0
 */

#include "Vec2FHelper.h"
#include <cmath>

double Vec2FHelper::distance(const ::Vec2F &from, const ::Vec2F &to) {
    return std::sqrt((from.x - to.x) * (from.x - to.x) + (from.y - to.y) * (from.y - to.y));
}

::Vec2F Vec2FHelper::midpoint(const ::Vec2F &from, const ::Vec2F &to) {
    return Vec2F((from.x + to.x) / 2.0, (from.y + to.y) / 2.0);
}

::Vec2F Vec2FHelper::rotate(const Vec2F &p, const Vec2F &origin, float angleDegree) {
    double sinAngle = sin(angleDegree * M_PI / 180.0);
    double cosAngle = cos(angleDegree * M_PI / 180.0);
    float x = p.x - origin.x;
    float y = p.y - origin.y;
    float rX = x * cosAngle - y * sinAngle;
    float rY = x * sinAngle + y * cosAngle;
    return Vec2F(rX + origin.x, rY + origin.y);
}

double Vec2FHelper::squaredLength(const ::Vec2F &vector) {
    return vector * vector;
}

Vec2F Vec2FHelper::normalize(const ::Vec2F &vector) {
    return vector / std::sqrt(Vec2FHelper::squaredLength(vector));
}

Vec2F operator+( const ::Vec2F& left, const ::Vec2F& right ) {
    return Vec2F(left.x + right.x, left.y + right.y);
}

Vec2F operator-( const ::Vec2F& left, const ::Vec2F& right ) {
    return Vec2F(left.x - right.x, left.y - right.y);
}

double operator*( const ::Vec2F& left, const ::Vec2F& right ) {
    return left.x * right.x + left.y * right.y;
}

Vec2F operator*( const ::Vec2F& left, const float& val ) {
    return Vec2F(left.x * val, left.y * val);
}

Vec2F operator/( const ::Vec2F& left, const double& val ) {
    return Vec2F(left.x / val, left.y / val);
}

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

class Vec2DHelper {
  public:
    static double distance(const ::Vec2D &from, const ::Vec2D &to);
    static double distanceSquared(const ::Vec2D &from, const ::Vec2D &to);

    static ::Vec2D midpoint(const ::Vec2D &from, const ::Vec2D &to);

    static ::Vec2D rotate(const ::Vec2D &p, const ::Vec2D &origin, double angleDegree);

    static double squaredLength(const ::Vec2D &vector);

    static ::Vec2D normalize(const ::Vec2D &vector);
};

Vec2D operator+( const ::Vec2D& left, const ::Vec2D& right );
Vec2D operator-( const ::Vec2D& left, const ::Vec2D& right );

Vec2D operator/( const ::Vec2D& left, const double& val );

// Overloading "*" operator for dot product.
double operator*( const ::Vec2D& left, const ::Vec2D& right );

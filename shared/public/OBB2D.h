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

#include "Quad2dD.h"
#include "Vec2D.h"
#include "Vec2DHelper.h"

class OBB2D {
private:
    /** Corners of the box, where 0 is the lower left. */
    Vec2D corner[4];

    /** Two edges of the box extended away from corner[0]. */
    Vec2D axis[2];

    /** Circle estimate as approach for fast rejection in overlap */
    float r = 0.0;
    Vec2D center = Vec2D(0.0,0.0);

    /** origin[a] = corner[0].dot(axis[a]); */
    double origin[2];

    /** Returns true if other overlaps one dimension of this. */
    inline bool overlaps1Way(const OBB2D& other) const {
        for (int a = 0; a < 2; ++a) {

            double t = other.corner[0].x * axis[a].x + other.corner[0].y * axis[a].y;

            // Find the extent of box 2 on axis a
            double tMin = t;
            double tMax = t;

            t = other.corner[1].x * axis[a].x + other.corner[1].y * axis[a].y;

            if (t < tMin) {
                tMin = t;
            } else if (t > tMax) {
                tMax = t;
            }

            t = other.corner[2].x * axis[a].x + other.corner[2].y * axis[a].y;

            if (t < tMin) {
                tMin = t;
            } else if (t > tMax) {
                tMax = t;
            }

            t = other.corner[3].x * axis[a].x + other.corner[3].y * axis[a].y;

            if (t < tMin) {
                tMin = t;
            } else if (t > tMax) {
                tMax = t;
            }

            // We have to subtract off the origin

            // See if [tMin, tMax] intersects [0, 1]
            if ((tMin > 1 + origin[a]) || (tMax < origin[a])) {
                // There was no intersection along this dimension;
                // the boxes cannot possibly overlap.
                return false;
            }
        }

        // There was no dimension along which there is no intersection.
        // Therefore the boxes overlap.
        return true;
    }

    inline bool overlapsCircles(const OBB2D& other) const {
        auto rs = (r + other.r);
        auto x = center.x - other.center.x;
        auto y = center.y - other.center.y;
        return x*x + y*y < rs*rs;
    }

public:

    OBB2D(const Quad2dD& quad) :
        corner { quad.bottomLeft, quad.bottomRight, quad.topRight, quad.topLeft },
        axis { corner[1] - corner[0], corner[3] - corner[0] }
    {
        // Make the length of each axis 1/edge length so we know any
        // dot product must be less than 1 to fall within the edge.
        for (int a = 0; a < 2; ++a) {
            axis[a] = axis[a] / Vec2DHelper::squaredLength(axis[a]);
            origin[a] = corner[0].x * axis[a].x + corner[0].y * axis[a].y;
        }

        auto a = Vec2DHelper::distance(corner[2], corner[0]);
        auto b = Vec2DHelper::distance(corner[3], corner[1]);

        if(a > b) {
            center = Vec2DHelper::midpoint(corner[2], corner[0]);
            r = a * 0.5;
        } else {
            center = Vec2DHelper::midpoint(corner[3], corner[1]);
            r = b * 0.5;
        }
    }

    /** Returns true if the intersection of the boxes is non-empty. */
    inline bool overlaps(const OBB2D& other) const {
        return overlapsCircles(other) && overlaps1Way(other) && other.overlaps1Way(*this);
    }
};

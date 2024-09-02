/*
 * Copyright (c) 2024 Ubique Innovation AG <https://www.ubique.ch>
 *
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this
 *  file, You can obtain one at https://mozilla.org/MPL/2.0/.
 *
 *  SPDX-License-Identifier: MPL-2.0
 */

#pragma once

#include "Vec3D.h"
#include <cmath>

class MapCamera3DHelper {
public:
    static float halfWidth(float cameraDistance, float fovy, float width, float height) {
        // According to https://en.wikipedia.org/wiki/Field_of_view_in_video_games, this holds:
        // tan(fovx/2.0) / tan(fovy/2.0) = w / h
        float fovyRadian = fovy * (M_PI / 180.0);
        float fovx = 2.0 * atan2(width, height / tan(fovyRadian * 0.5));
        fovx = fovx * (180.0 / M_PI);

        return halfHeight(cameraDistance, fovx);
    }

    static float halfHeight(float cameraDistance, float fovy) {
        return cameraDistance * tan((fovy * 0.5) * (M_PI / 180.0));
    }

    static Vec3D raySphereIntersection(const Vec3D& rayStart, const Vec3D &rayEnd, const Vec3D &sphereCenter, float radius, bool &didHit) {

        // https://www.khoury.northeastern.edu/home/fell/CS4300/Lectures/Ray-TracingFormulas.pdf
        float dx = rayEnd.x - rayStart.x;
        float dy = rayEnd.y - rayStart.y;
        float dz = rayEnd.z - rayStart.z;

        float cx = sphereCenter.x;
        float cy = sphereCenter.y;
        float cz = sphereCenter.z;

        float x0 = rayStart.x;
        float y0 = rayStart.y;
        float z0 = rayStart.z;

        auto a = dx*dx + dy*dy + dz*dz;
        auto b = 2.0 * dx * (x0 - cx) + 2.0 * dy * (y0 - cy) + 2.0 * dz * (z0 - cz);
        auto c = cx*cx + cy*cy + cz*cz + x0*x0 + y0*y0 + z0*z0 - 2 * (cx*x0 + cy*y0 + cz*z0) - radius * radius;

        auto disc = b*b - 4*a*c;

        if(disc > 0) {
            didHit = true;
            auto t = (-b - sqrt(b*b - 4*a*c)) / (2*a);
            return Vec3D(x0 + t * dx, y0 + t * dy, z0 + t * dz);
        } else {
            didHit = false;
            return rayEnd;
        }
    }
};

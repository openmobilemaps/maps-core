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

#include "Matrix.h"
#include "MatrixD.h"
#include "RectD.h"
#include "RectD.h"
#include "Vec3D.h"
#include "CircleD.h"
#include "CollisionPrimitives.h"
#include <vector>
#include "Vec4D.h"

class CollisionUtil {

public:
    static bool checkRectCollision(const RectD &rect1, const RectD &rect2, int32_t addSpacing = 0) {
        return (rect1.x < (rect2.x + rect2.width + addSpacing)) &&
               ((rect1.x + rect1.width) > (rect2.x - addSpacing)) &&
               (rect1.y < (rect2.y + rect2.height + addSpacing)) &&
               ((rect1.y + rect1.height) > (rect2.y - addSpacing));
    }

    static bool checkCircleCollision(const CircleD &circle1, const CircleD &circle2, int32_t addSpacing = 0) {
        double dX = circle1.x - circle2.x;
        double dY = circle1.y - circle2.y;
        double distanceSq = dX * dX + dY * dY;
        double r = circle1.radius + circle2.radius + addSpacing;
        return distanceSq < (r * r);
    }

    static bool checkRectCircleCollision(const CollisionRectD &rect, const CircleD &circle, int32_t addSpacing = 0) {
        double minX = std::min(rect.x + rect.width, rect.x);
        double minY = std::min(rect.y + rect.height, rect.y);
        double closestX = std::max(minX, std::min(minX + rect.width, circle.x));
        double closestY = std::max(minY, std::min(minY + rect.height, circle.y));
        double dX = closestX - circle.x;
        double dY = closestY - circle.y;
        double r = circle.radius + addSpacing;
        return (dX * dX + dY * dY) < (r * r);
    }

    static bool checkRectCircleCollision(const RectD &rect, const CircleD &circle, int32_t addSpacing = 0) {
        double minX = std::min(rect.x + rect.width, rect.x);
        double minY = std::min(rect.y + rect.height, rect.y);
        double closestX = std::max(minX, std::min(minX + rect.width, circle.x));
        double closestY = std::max(minY, std::min(minY + rect.height, circle.y));
        double dX = closestX - circle.x;
        double dY = closestY - circle.y;
        double r = circle.radius + addSpacing;
        return (dX * dX + dY * dY) < (r * r);
    }

    static bool checkCirclesCollision(const std::vector<CircleD> &circles, const CircleD &circle2, int32_t addSpacing = 0) {
        for (const auto &circle1 : circles) {
            double dX = circle1.x - circle2.x;
            double dY = circle1.y - circle2.y;
            double distanceSq = dX * dX + dY * dY;
            double r = circle1.radius + circle2.radius + addSpacing;
            if (distanceSq < (r * r)) {
                return true;
            }
        }
        return false;
    }

    struct CollisionEnvironment {
        const std::vector<double> &vpMatrix;
        const bool is3d;
        Vec4D &temp1;
        Vec4D &temp2;
        const double halfWidth;
        const double halfHeight;
        const double sinNegGridAngle;
        const double cosNegGridAngle;
        const Vec3D &origin;

        CollisionEnvironment(
                             const std::vector<double>& vpMatrix,
                             const bool is3d,
                             Vec4D& temp1,
                             Vec4D& temp2,
                             const double halfWidth,
                             const double halfHeight,
                             const double sinNegGridAngle,
                             const double cosNegGridAngle,
                             const Vec3D &origin)
        : vpMatrix(vpMatrix),
        is3d(is3d),
        temp1(temp1),
        temp2(temp2),
        halfWidth(halfWidth),
        halfHeight(halfHeight),
        sinNegGridAngle(sinNegGridAngle),
        cosNegGridAngle(cosNegGridAngle),
        origin(origin)
        {}
    };

    static std::optional<CircleD> getProjectedCircle(const CollisionCircleD &circle, CollisionEnvironment &env) {
        if (env.is3d) {
            // Earth center
            env.temp2.x = 1.0 * sin(0.0) * cos(0.0) - env.origin.x;
            env.temp2.y = 1.0 * cos(0.0) - env.origin.y;
            env.temp2.z = -1.0 * sin(0.0) * sin(0.0) - env.origin.z;
            env.temp2.w = 1.0;


            MatrixD::multiply(env.vpMatrix, env.temp2, env.temp1);

            double earthCenterZ = env.temp1.z / env.temp1.w;

            env.temp2.x = 1.0 * sin(circle.y) * cos(circle.x) - env.origin.x;
            env.temp2.y = 1.0 * cos(circle.y) - env.origin.y;
            env.temp2.z = -1.0 * sin(circle.y) * sin(circle.x) - env.origin.z;
            env.temp2.w = 1.0;

            MatrixD::multiply(env.vpMatrix, env.temp2, env.temp1);

            env.temp1.x /= env.temp1.w;
            env.temp1.y /= env.temp1.w;
            env.temp1.z /= env.temp1.w;
            env.temp1.w /= env.temp1.w;

            auto diffCenterZ = env.temp1.z - earthCenterZ;

            if (diffCenterZ > 0) {
                return std::nullopt;
            }

            double originX = env.temp1.x * env.halfWidth + env.halfWidth;
            double originY = env.temp1.y * env.halfHeight + env.halfHeight;

            return CircleD {originX, originY, circle.radius};
        } else {
            env.temp2.x = circle.x - env.origin.x;
            env.temp2.y = circle.y - env.origin.y;
            env.temp2.z = 0.0;
            env.temp2.w = 1.0;

            MatrixD::multiply(env.vpMatrix, env.temp2, env.temp1);

            double originX = (env.temp1.x / env.temp1.w) * env.halfWidth + env.halfWidth;
            double originY = (env.temp1.y / env.temp1.w) * env.halfHeight + env.halfHeight;

            env.temp2.x = circle.radius;
            env.temp2.y = circle.radius;
            env.temp2.z = 0.0;
            env.temp2.w = 0.0;

            MatrixD::multiply(env.vpMatrix, env.temp2, env.temp1);
            env.temp1.x = env.temp1.x * env.halfWidth;
            env.temp1.y = env.temp1.y * env.halfHeight;

            double iRadius = std::sqrt(env.temp1.x * env.temp1.x + env.temp1.y * env.temp1.y);
            return CircleD {originX, originY, iRadius};
        }
    }

    static std::optional<RectD> getProjectedRectangle(const CollisionRectD &rectangle, CollisionEnvironment &env) {
        if (env.is3d) {
            // Earth center
            env.temp2.x = 1.0 * sin(0.0) * cos(0.0) - env.origin.x;
            env.temp2.y = 1.0 * cos(0.0) - env.origin.y;
            env.temp2.z = -1.0 * sin(0.0) * sin(0.0) - env.origin.z;
            env.temp2.w = 1.0;

            MatrixD::multiply(env.vpMatrix, env.temp2, env.temp1);

            double earthCenterZ = env.temp1.z / env.temp1.w;

            env.temp2.x = 1.0 * sin(rectangle.anchorY) * cos(rectangle.anchorX) - env.origin.x;
            env.temp2.y = 1.0 * cos(rectangle.anchorY) - env.origin.y;
            env.temp2.z = -1.0 * sin(rectangle.anchorY) * sin(rectangle.anchorX) - env.origin.z;
            env.temp2.w = 1.0;

            MatrixD::multiply(env.vpMatrix, env.temp2, env.temp1);

            env.temp1.x /= env.temp1.w;
            env.temp1.y /= env.temp1.w;
            env.temp1.z /= env.temp1.w;
            env.temp1.w /= env.temp1.w;

            auto diffCenterZ = env.temp1.z - earthCenterZ;

            if (diffCenterZ > 0) {
                return std::nullopt;
            }

            double originX = (env.temp1.x * env.halfWidth + env.halfWidth);
            double originY = (env.temp1.y * env.halfHeight + env.halfHeight);

            originX += rectangle.x;
            originY -= rectangle.y;

            double w = rectangle.width;
            double h = rectangle.height;

            return RectD {originX , originY - h, std::abs(w), std::abs(h)};
        } else {
            env.temp2.x = (rectangle.x - env.origin.x) - rectangle.anchorX;
            env.temp2.y = (rectangle.y - env.origin.y) - rectangle.anchorY;
            env.temp2.z = 0.0;
            env.temp2.w = 1.0;

            // Rotate and move to the correct position
            double rotatedX = env.temp2.x * env.cosNegGridAngle - env.temp2.y * env.sinNegGridAngle;
            double rotatedY = env.temp2.x * env.sinNegGridAngle + env.temp2.y * env.cosNegGridAngle;

            env.temp2.x = rotatedX + rectangle.anchorX;
            env.temp2.y = rotatedY + rectangle.anchorY;

            MatrixD::multiply(env.vpMatrix, env.temp2, env.temp1);

            double originX = (env.temp1.x / env.temp1.w) * env.halfWidth + env.halfWidth;
            double originY = (env.temp1.y / env.temp1.w) * env.halfHeight + env.halfHeight;

            // Calculate rotated width
            env.temp2.x = rectangle.width * env.cosNegGridAngle;
            env.temp2.y = rectangle.width * env.sinNegGridAngle;
            env.temp2.z = 0.0;
            env.temp2.w = 0.0;

            MatrixD::multiply(env.vpMatrix, env.temp2, env.temp1);
            double w = env.temp1.x;
            double h = env.temp1.y;

            // Calculate rotated height
            env.temp2.x = -rectangle.height * env.sinNegGridAngle;
            env.temp2.y = rectangle.height * env.cosNegGridAngle;
            env.temp2.z = 0.0;
            env.temp2.w = 0.0;

            MatrixD::multiply(env.vpMatrix, env.temp2, env.temp1);
            w += env.temp1.x;
            h += env.temp1.y;

            double width = w * env.halfWidth;
            double height = h * env.halfHeight;

            originX = std::min(originX, originX + width);
            originY = std::min(originY, originY + height);

            return RectD {originX, originY, std::abs(width), std::abs(height)};
        }
    }
};

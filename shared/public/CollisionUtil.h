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
#include "RectF.h"
#include "CircleF.h"
#include "CollisionPrimitives.h"
#include <vector>

class CollisionUtil {

public:
    static bool checkRectCollision(const RectF &rect1, const RectF &rect2, int32_t addSpacing = 0) {
        return (rect1.x < (rect2.x + rect2.width + addSpacing)) &&
               ((rect1.x + rect1.width) > (rect2.x - addSpacing)) &&
               (rect1.y < (rect2.y + rect2.height + addSpacing)) &&
               ((rect1.y + rect1.height) > (rect2.y - addSpacing));
    }

    static bool checkRectCircleCollision(const RectF &rect, const CircleF &circle, int32_t addSpacing = 0) {
        float minX = std::min(rect.x + rect.width, rect.x);
        float minY = std::min(rect.y + rect.height, rect.y);
        float closestX = std::max(minX, std::min(minX + rect.width, circle.x));
        float closestY = std::max(minY, std::min(minY + rect.height, circle.y));
        float dX = closestX - circle.x;
        float dY = closestY - circle.y;
        float r = circle.radius + addSpacing;
        return (dX * dX + dY * dY) < (r * r);
    }

    static bool checkCircleCollision(const CircleF &circle1, const CircleF &circle2, int32_t addSpacing = 0) {
        float dX = circle1.x - circle2.x;
        float dY = circle1.y - circle2.y;
        float distanceSq = dX * dX + dY * dY;
        float r = circle1.radius + circle2.radius + addSpacing;
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
};

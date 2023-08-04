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

struct CollisionRectD {
    double x;
    double y;
    double width;
    double height;
    size_t contentHash;
    double symbolSpacing; // In projected space of the CollisionGrid

    CollisionRectD(double x, double y, double width, double height, size_t contentHash = 0, double symbolSpacing = 0)
            : x(x), y(y), width(width), height(height), contentHash(contentHash), symbolSpacing(symbolSpacing) {}
};

struct CollisionRectI {
    int32_t x;
    int32_t y;
    int32_t width;
    int32_t height;
    size_t contentHash;
    int32_t symbolSpacing; // In projected space of the CollisionGrid

    CollisionRectI(int32_t x, int32_t y, int32_t width, int32_t height, size_t contentHash = 0, int32_t symbolSpacing = 0)
            : x(x), y(y), width(width), height(height), contentHash(contentHash), symbolSpacing(symbolSpacing) {}
};

struct CollisionCircleD {
    double x;
    double y;
    double radius;
    size_t contentHash;
    double symbolSpacing; // In projected space of the CollisionGrid

    CollisionCircleD(double x, double y, double radius, size_t contentHash = 0, double symbolSpacing = 0)
            : x(x), y(y), radius(radius), contentHash(contentHash), symbolSpacing(symbolSpacing) {}
};

struct CollisionCircleI {
    int32_t x;
    int32_t y;
    int32_t radius;
    size_t contentHash;
    int32_t symbolSpacing; // In projected space of the CollisionGrid

    CollisionCircleI(int32_t x, int32_t y, int32_t radius, size_t contentHash = 0, int32_t symbolSpacing = 0)
            : x(x), y(y), radius(radius), contentHash(contentHash), symbolSpacing(symbolSpacing) {}
};

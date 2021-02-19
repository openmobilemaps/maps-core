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
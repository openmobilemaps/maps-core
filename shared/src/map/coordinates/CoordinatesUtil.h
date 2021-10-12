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

#include "QuadCoord.h"

namespace coordsutil {

    static bool checkIntersectionRectCoords2d(const RectCoord &r1, const RectCoord &r2) {
        assert(r1.topLeft.systemIdentifier == r2.topLeft.systemIdentifier);
        return !(std::min(r2.topLeft.x, r2.bottomRight.x) > std::max(r1.topLeft.x, r1.bottomRight.x) ||
                 std::max(r2.topLeft.x, r2.bottomRight.x) < std::min(r1.topLeft.x, r1.bottomRight.x) ||
                 std::min(r2.topLeft.y, r2.bottomRight.y) > std::max(r1.topLeft.y, r1.bottomRight.y) ||
                 std::max(r2.topLeft.y, r2.bottomRight.y) < std::min(r1.topLeft.y, r1.bottomRight.y));
    }

}
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

// https://gist.github.com/volkansalma/2972237
float atan2_approximation(float y, float x)
{
    float abs_y = std::fabs(y) + 1e-10f;      // kludge to prevent 0/0 condition
    float r = (x - std::copysign(abs_y, x)) / (abs_y + std::fabs(x));
    float angle = M_PI/2.f - std::copysign(M_PI/4.f, x);
    angle += (0.1963f * r * r - 0.9817f) * r;
    return std::copysign(angle, y);
}

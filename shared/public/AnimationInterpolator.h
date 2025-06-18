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

#include <cmath>
#include <utility>

enum InterpolatorFunction {
    Linear,
    EaseIn,
    EaseOut,
    EaseInOut,
    EaseInBounce,
    EaseOutBounce
};

struct AnimationInterpolator {
public:

    AnimationInterpolator(InterpolatorFunction function): function(function){}

    double interpolate(double value) {
        switch (function) {
            case InterpolatorFunction::Linear:
                return  value;
            case InterpolatorFunction::EaseIn:
                return value * value;
            case InterpolatorFunction::EaseOut:
                return value * (2 - value);
            case InterpolatorFunction::EaseInOut:
                return value < 0.5 ? 2 * value * value : value * (4 - 2 * value) - 1;
            case InterpolatorFunction::EaseInBounce:
                return pow( 2, 6 * (value - 1) ) * std::abs( sin( value * M_PI * 3.5 ) );
            case InterpolatorFunction::EaseOutBounce:
                return 1 - pow( 2, -6 * value ) * std::abs( cos( value * M_PI * 3.5 ) );
            default:
#if __cplusplus >= 202302L
                std::unreachable();
#else
                __builtin_unreachable();
#endif
        }
    }
;
private:
    InterpolatorFunction function;
};

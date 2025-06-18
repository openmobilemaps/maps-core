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
#include "RasterShaderStyle.h"
#include "DefaultAnimator.h"

class RasterStyleAnimation: public DefaultAnimator<RasterShaderStyle> {
public:
    RasterStyleAnimation(long long duration,
                    RasterShaderStyle startValue,
                    RasterShaderStyle endValue,
                    InterpolatorFunction interpolatorFunction,
                    std::function<void(RasterShaderStyle)> onUpdate,
                    std::optional<std::function<void()>> onFinish = std::nullopt):
    DefaultAnimator<RasterShaderStyle>(duration, startValue, endValue, interpolatorFunction, onUpdate, onFinish) {}

    virtual void update(double adjustedProgress) override {
        onUpdate(RasterShaderStyle(startValue.opacity + (endValue.opacity - startValue.opacity) * adjustedProgress, 
                                   startValue.brightnessMin + (endValue.brightnessMin - startValue.brightnessMin) * adjustedProgress, 
                                   startValue.brightnessMax + (endValue.brightnessMax - startValue.brightnessMax) * adjustedProgress, 
                                   startValue.contrast + (endValue.contrast - startValue.contrast) * adjustedProgress,
                                   startValue.saturation + (endValue.saturation - startValue.saturation) * adjustedProgress,
                                   startValue.gamma + (endValue.gamma - startValue.gamma) * adjustedProgress,
                                   startValue.brightnessShift + (endValue.brightnessShift - startValue.brightnessShift) * adjustedProgress));
    };
};



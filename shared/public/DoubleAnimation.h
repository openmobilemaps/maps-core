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
#include "DefaultAnimator.h"

class DoubleAnimation: public DefaultAnimator<double> {
public:
    DoubleAnimation(long long duration,
                     double startValue,
                    double endValue,
                    InterpolatorFunction interpolatorFunction,
                     std::function<void(double)> onUpdate,
                    std::optional<std::function<void()>> onFinish = std::nullopt,
                    bool usesDegrees = false):
    DefaultAnimator<double>(duration, startValue, endValue, interpolatorFunction, onUpdate, onFinish),
    usesDegrees(usesDegrees){}


    virtual void update(double adjustedProgress) override {
        if (usesDegrees) {
            double currentValue = startValue + (fmod(fmod(endValue - startValue, 360) + 540, 360) - 180) * adjustedProgress;
            onUpdate(currentValue);
        } else {
            double currentValue = startValue + (endValue - startValue) * adjustedProgress;
            onUpdate(currentValue);
        }
    };
    
protected:
    bool usesDegrees;
};



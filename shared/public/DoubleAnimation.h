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
                    std::optional<std::function<void()>> onFinish = std::nullopt):
    DefaultAnimator<double>(duration, startValue, endValue, interpolatorFunction, onUpdate, onFinish) {}


    virtual void update(double adjustedProgress) override {

        double currentValue = startValue + (endValue - startValue) * adjustedProgress;

        onUpdate(currentValue);
        
    };
};



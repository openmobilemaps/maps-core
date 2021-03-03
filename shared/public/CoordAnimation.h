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
#include "Coord.h"

class CoordAnimation: public DefaultAnimator<Coord> {
public:
     CoordAnimation(long long duration,
                    Coord startValue,
                    Coord endValue,
                    InterpolatorFunction interpolatorFunction,
                    std::function<void(Coord)> onUpdate,
                    std::optional<std::function<void()>> onFinish = std::nullopt):
    DefaultAnimator<Coord>(duration, startValue, endValue, interpolatorFunction, onUpdate, onFinish) {
        assert(startValue.systemIdentifier == endValue.systemIdentifier);
    }

    virtual void update(double adjustedProgress) override {

        double currentXValue = startValue.x + (endValue.x - startValue.x) * adjustedProgress;
        double currentYValue = startValue.y + (endValue.y - startValue.y) * adjustedProgress;
        double currentZValue = startValue.z + (endValue.z - startValue.z) * adjustedProgress;

        auto coord = Coord(startValue.systemIdentifier, currentXValue, currentYValue, currentZValue);

        onUpdate(coord);

    };
};



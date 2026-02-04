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
#include <functional>
#include <algorithm>
#include <optional>
#include "DateHelper.h"
#include "AnimationInterpolator.h"
#include "AnimationInterface.h"

template <typename T>
class DefaultAnimator: public AnimationInterface {
public:
    DefaultAnimator(int64_t duration,
                     T startValue,
                     T endValue,
                     InterpolatorFunction interpolatorFunction,
                     std::function<void(T)> onUpdate,
                     std::optional<std::function<void()>> onFinish = std::nullopt):
    duration(duration),
    startValue(startValue),
    endValue(endValue),
    interpolator(interpolatorFunction),
    onUpdate(onUpdate),
    onFinish(onFinish) {};

    virtual void start() override {
        start(0);
    };

    virtual void start(int64_t delay) override {
        startTime = DateHelper::currentTimeMillis();
        this->delay = delay;
        animationState = State::started;
    };

    virtual void cancel() override {
        animationState = State::canceled;
    };

    virtual void finish() override {
        animationState = State::finished;

        if (onFinish != std::nullopt) {
            (*onFinish)();
        }
    };

    virtual bool isFinished() override {
        return animationState == State::finished || animationState == State::canceled;
    };

    virtual void update() override {
        if (animationState != State::started) { return; }

        auto currentTimeStamp = DateHelper::currentTimeMillis();

        if (startTime + delay > currentTimeStamp) {
            update(0);
            return;
        }

        double progress = std::max(std::min(((double)(currentTimeStamp - (startTime + delay)) / duration), 1.0), 0.0);

        double adjustedProgress = interpolator.interpolate(progress);

        update(adjustedProgress);

        if (progress >= 1) {
            finish();
        }
    };

    virtual void update(double adjustedProgress) = 0;

    T startValue;
    T endValue;

protected:
    int64_t duration;
    int64_t startTime = 0;
    int64_t delay = 0;

    AnimationInterpolator interpolator;
    std::function<void(T)> onUpdate;
    std::optional<std::function<void()>> onFinish;
    
    enum State { created, started, canceled, finished };
    State animationState = State::created;
};


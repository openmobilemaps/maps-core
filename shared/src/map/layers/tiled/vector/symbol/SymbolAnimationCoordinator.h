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

#include "AnimationInterpolator.h"
#include "Tiled2dMapTileInfo.h"
#include <atomic>

#define COLLISION_ANIMATION_DURATION_MS 300.0f

class SymbolAnimationCoordinator {
public:
    SymbolAnimationCoordinator(): interpolator(InterpolatorFunction::EaseInOut) {}

    float getIconAlpha(float targetAlpha, long long now) {
        return internalGetAlpha(targetAlpha, now, lastIconAlpha, iconAnimationStart);
    }

    float getStretchIconAlpha(float targetAlpha, long long now) {
        return internalGetAlpha(targetAlpha, now, lastStretchIconAlpha, stretchIconAnimationStart);
    }

    float getTextAlpha(float targetAlpha, long long now) {
        return internalGetAlpha(targetAlpha, now, lastTextAlpha, textAnimationStart);
    }

    bool isAnimating() {
        return isIconAnimating() || isStretchIconAnimating() || isTextAnimating();
    }

    bool isIconAnimating() {
        return iconAnimationStart != 0;
    }

    bool isTextAnimating() {
        return textAnimationStart != 0;
    }

    bool isStretchIconAnimating() {
        return stretchIconAnimationStart != 0;
    }

    bool isUsed() {
        return usageCount > 0;
    }

    int getUsageCount() {
        return usageCount;
    }

    int increaseUsage() {
        usageCount++;
        return usageCount;
    }

    int decreaseUsage() {
        usageCount--;
        return usageCount;
    }

    std::atomic_flag isOwned = ATOMIC_FLAG_INIT;

    bool isColliding = true;

private:
    long long iconAnimationStart = 0;
    float lastIconAlpha = 0;

    long long stretchIconAnimationStart = 0;
    float lastStretchIconAlpha = 0;

    long long textAnimationStart = 0;
    float lastTextAlpha = 0;

    int usageCount = 0;

    AnimationInterpolator interpolator;

    float internalGetAlpha(float targetAlpha, long long now, float &lastAlpha, long long &animationStart) {
        if (lastAlpha != targetAlpha) {
            if (animationStart == 0) {
                animationStart = now;
            }
            float progress = std::min(double(now - animationStart) / COLLISION_ANIMATION_DURATION_MS, 1.0);
            if (progress == 1.0) {
                animationStart = 0;
                lastAlpha = targetAlpha;
                return targetAlpha;
            }
            return lastAlpha + (targetAlpha - lastAlpha) * interpolator.interpolate(progress);
        }
        animationStart = 0;
        return targetAlpha;
    }
};

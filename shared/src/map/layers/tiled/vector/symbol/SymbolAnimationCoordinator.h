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
#include "Coord.h"
#include <atomic>

class SymbolAnimationCoordinator {
public:
    SymbolAnimationCoordinator(const Coord &coordinate,
                               const int zoomIdentifier,
                               const double xTolerance,
                               const double yTolerance,
                               const int64_t animationDuration,
                               const int64_t animationDelay):
    interpolator(InterpolatorFunction::EaseInOut),
    coordinate(coordinate),
    zoomIdentifier(zoomIdentifier),
    xTolerance(xTolerance),
    yTolerance(yTolerance),
    animationDuration(animationDuration),
    animationDelay(animationDelay){}

    bool isMatching(const Coord &coordinate, const int zoomIdentifier) const {
        const double toleranceFactor = 1 << std::max(0, this->zoomIdentifier - zoomIdentifier); // more efficient than: std::max(1.0, std::pow(2, this->zoomIdentifier - zoomIdentifier))
        const double xDistance = std::abs(this->coordinate.x - coordinate.x);
        const double yDistance = std::abs(this->coordinate.y - coordinate.y);
        const bool matching = xDistance <= xTolerance * toleranceFactor && yDistance <= yTolerance * toleranceFactor;
        return matching;
    }

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
        if (cacheCount >= usageCount) {
            return false;
            
        }
        if (isIconAnimating() || isStretchIconAnimating() || isTextAnimating()) {
            return true;
        }
        else {
            return false;
        }
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

    int increaseUsage() {;
        return ++usageCount;
    }

    int decreaseUsage() {
        return --usageCount;
    }

    int increaseCache() {;
        return ++cacheCount;
    }

    int decreaseCache() {
        return --cacheCount;
    }

    std::atomic_flag isOwned = ATOMIC_FLAG_INIT;

    // returns true if the value was changed
    bool setColliding(const bool isColliding) {
        const bool wasChanged = collides != isColliding;
        collides = isColliding;
        return wasChanged;
    }

    bool isColliding() {
        return collides;
    }

    const Coord coordinate;
    const int zoomIdentifier;
private:
    const double xTolerance;
    const double yTolerance;

    long long iconAnimationStart = 0;
    float lastIconAlpha = 0;

    long long stretchIconAnimationStart = 0;
    float lastStretchIconAlpha = 0;

    long long textAnimationStart = 0;
    float lastTextAlpha = 0;

    std::atomic_int usageCount = 0;
    std::atomic_int cacheCount = 0;

    bool collides = true;

    AnimationInterpolator interpolator;

    const int64_t animationDuration;
    const int64_t animationDelay;

    float internalGetAlpha(float targetAlpha, long long now, float &lastAlpha, long long &animationStart) {
        if (animationDuration == 0) {
            return targetAlpha;
        }
        if (lastAlpha != targetAlpha) {
            if (animationStart == 0) {
                animationStart = now + animationDelay;
            }
            float progress = std::min(double(std::max(now - animationStart, 0ll)) / animationDuration, 1.0);
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

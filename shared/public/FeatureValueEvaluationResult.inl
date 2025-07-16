/*
 * Copyright (c) 2025 Ubique Innovation AG <https://www.ubique.ch>
 *
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this
 *  file, You can obtain one at https://mozilla.org/MPL/2.0/.
 *
 *  SPDX-License-Identifier: MPL-2.0
 */

#pragma once

#include "Value.h"

template<typename T>
bool FeatureValueEvaluationResult<T>::isReevaluationNeeded(const EvaluationContext& context) const {
    switch (policy) {
        case ReevaluationPolicy::NEVER:
            return false;
        case ReevaluationPolicy::ALWAYS:
            return true;
        case ReevaluationPolicy::ZOOM:
            return context.zoomLevel && zoomRange.contains(*context.zoomLevel);
        case ReevaluationPolicy::STATE:
            return context.featureStateManager &&
                   stateId != context.featureStateManager->getCurrentState();
        case ReevaluationPolicy::ZOOM_AND_STATE:
            return (context.zoomLevel && zoomRange.contains(*context.zoomLevel)) ||
                   (context.featureStateManager &&
                    stateId != context.featureStateManager->getCurrentState());
    }
    return true; // fallback
}

template<typename T>
void FeatureValueEvaluationResult<T>::invalidate() {
    policy = ReevaluationPolicy::ALWAYS;
    stateId = -1;
    zoomRange.setFullRange();
}

template<typename T>
FeatureValueEvaluationResult<T>::FeatureValueEvaluationResult(const T& val)
    : value(val) {}

template<typename T>
FeatureValueEvaluationResult<T>::FeatureValueEvaluationResult(T&& val)
    : value(std::move(val)) {}

template<typename T>
FeatureValueEvaluationResult<T>::FeatureValueEvaluationResult(const T& val,
                                     ReevaluationPolicy policy,
                                     int32_t stateId,
                                     const ZoomRange& range)
    : value(val), policy(policy), stateId(stateId), zoomRange(range) {}

template<typename T>
FeatureValueEvaluationResult<T>::FeatureValueEvaluationResult(T&& val,
                                     ReevaluationPolicy policy,
                                     int32_t stateId,
                                     const ZoomRange& range)
    : value(std::move(val)), policy(policy), stateId(stateId), zoomRange(range) {}

template<typename T>
FeatureValueEvaluationResult<T> FeatureValueEvaluationResult<T>::constant(const T& val) {
    return FeatureValueEvaluationResult(val, ReevaluationPolicy::NEVER, -1, ZoomRange());
}

template<typename T>
FeatureValueEvaluationResult<T> FeatureValueEvaluationResult<T>::constant(T&& val) {
    return FeatureValueEvaluationResult(std::move(val), ReevaluationPolicy::NEVER, -1, ZoomRange());
}

template<typename T>
FeatureValueEvaluationResult<T> FeatureValueEvaluationResult<T>::always(T&& val) {
    return FeatureValueEvaluationResult(std::move(val), ReevaluationPolicy::ALWAYS, -1, ZoomRange());
}

template<typename T>
FeatureValueEvaluationResult<T> FeatureValueEvaluationResult<T>::zoomOnly(T&& val, const ZoomRange& range) {
    return FeatureValueEvaluationResult(std::move(val), ReevaluationPolicy::ZOOM, -1, range);
}

template<typename T>
FeatureValueEvaluationResult<T> FeatureValueEvaluationResult<T>::stateOnly(T&& val, int32_t stateId) {
    return FeatureValueEvaluationResult(std::move(val), ReevaluationPolicy::STATE, stateId, ZoomRange());
}

template<typename T>
FeatureValueEvaluationResult<T> FeatureValueEvaluationResult<T>::zoomAndState(T&& val, int32_t stateId, const ZoomRange& range) {
    return FeatureValueEvaluationResult(std::move(val), ReevaluationPolicy::ZOOM_AND_STATE, stateId, range);
}

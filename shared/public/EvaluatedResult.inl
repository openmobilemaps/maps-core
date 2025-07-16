//
//  EvaluatedResult.inl
//  MapCore
//
//  Created by Marco Zimmermann on 16.07.2025.
//

#pragma once

#include "Value.h"

template<typename T>
bool EvaluatedResult<T>::isReevaluationNeeded(const EvaluationContext& context) const {
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
void EvaluatedResult<T>::invalidate() {
    policy = ReevaluationPolicy::ALWAYS;
    stateId = -1;
    zoomRange.setFullRange();
}

template<typename T>
EvaluatedResult<T>::EvaluatedResult(const T& val)
    : value(val) {}

template<typename T>
EvaluatedResult<T>::EvaluatedResult(T&& val)
    : value(std::move(val)) {}

template<typename T>
EvaluatedResult<T>::EvaluatedResult(const T& val,
                                     ReevaluationPolicy policy,
                                     int32_t stateId,
                                     const ZoomRange& range)
    : value(val), policy(policy), stateId(stateId), zoomRange(range) {}

template<typename T>
EvaluatedResult<T>::EvaluatedResult(T&& val,
                                     ReevaluationPolicy policy,
                                     int32_t stateId,
                                     const ZoomRange& range)
    : value(std::move(val)), policy(policy), stateId(stateId), zoomRange(range) {}

template<typename T>
EvaluatedResult<T> EvaluatedResult<T>::constant(const T& val) {
    return EvaluatedResult(val, ReevaluationPolicy::NEVER, -1, ZoomRange());
}

template<typename T>
EvaluatedResult<T> EvaluatedResult<T>::constant(T&& val) {
    return EvaluatedResult(std::move(val), ReevaluationPolicy::NEVER, -1, ZoomRange());
}

template<typename T>
EvaluatedResult<T> EvaluatedResult<T>::always(T&& val) {
    return EvaluatedResult(std::move(val), ReevaluationPolicy::ALWAYS, -1, ZoomRange());
}

template<typename T>
EvaluatedResult<T> EvaluatedResult<T>::zoomOnly(T&& val, const ZoomRange& range) {
    return EvaluatedResult(std::move(val), ReevaluationPolicy::ZOOM, -1, range);
}

template<typename T>
EvaluatedResult<T> EvaluatedResult<T>::stateOnly(T&& val, int32_t stateId) {
    return EvaluatedResult(std::move(val), ReevaluationPolicy::STATE, stateId, ZoomRange());
}

template<typename T>
EvaluatedResult<T> EvaluatedResult<T>::zoomAndState(T&& val, int32_t stateId, const ZoomRange& range) {
    return EvaluatedResult(std::move(val), ReevaluationPolicy::ZOOM_AND_STATE, stateId, range);
}

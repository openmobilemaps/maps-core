// AUTOGENERATED FILE - DO NOT MODIFY!
// This file was generated by Djinni from core.djinni

#pragma once

#include "CameraInterpolation.h"
#include <cstdint>
#include <optional>
#include <string>
#include <utility>

struct Camera3dConfig final {
    std::string key;
    bool allowUserInteraction;
    std::optional<float> rotationSpeed;
    int32_t animationDurationMs;
    float minZoom;
    float maxZoom;
    CameraInterpolation pitchInterpolationValues;
    /** vertical camera displacement in padding-reduced viewport in [-1.0, 1.0] */
    CameraInterpolation verticalDisplacementInterpolationValues;

    Camera3dConfig(std::string key_,
                   bool allowUserInteraction_,
                   std::optional<float> rotationSpeed_,
                   int32_t animationDurationMs_,
                   float minZoom_,
                   float maxZoom_,
                   CameraInterpolation pitchInterpolationValues_,
                   CameraInterpolation verticalDisplacementInterpolationValues_)
    : key(std::move(key_))
    , allowUserInteraction(std::move(allowUserInteraction_))
    , rotationSpeed(std::move(rotationSpeed_))
    , animationDurationMs(std::move(animationDurationMs_))
    , minZoom(std::move(minZoom_))
    , maxZoom(std::move(maxZoom_))
    , pitchInterpolationValues(std::move(pitchInterpolationValues_))
    , verticalDisplacementInterpolationValues(std::move(verticalDisplacementInterpolationValues_))
    {}
};

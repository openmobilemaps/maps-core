// AUTOGENERATED FILE - DO NOT MODIFY!
// This file was generated by Djinni from common.djinni

#pragma once

#include <utility>

struct CircleF final {
    float x;
    float y;
    float radius;

    CircleF(float x_,
            float y_,
            float radius_)
    : x(std::move(x_))
    , y(std::move(y_))
    , radius(std::move(radius_))
    {}
};

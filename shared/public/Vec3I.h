// AUTOGENERATED FILE - DO NOT MODIFY!
// This file was generated by Djinni from common.djinni

#pragma once

#include <cstdint>
#include <utility>

struct Vec3I final {
    int32_t x;
    int32_t y;
    int32_t z;

    Vec3I(int32_t x_,
          int32_t y_,
          int32_t z_)
    : x(std::move(x_))
    , y(std::move(y_))
    , z(std::move(z_))
    {}
};

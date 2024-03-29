// AUTOGENERATED FILE - DO NOT MODIFY!
// This file was generated by Djinni from text.djinni

#pragma once

#include <functional>

enum class Anchor : int {
    CENTER = 0,
    LEFT = 1,
    RIGHT = 2,
    TOP = 3,
    BOTTOM = 4,
    TOP_LEFT = 5,
    TOP_RIGHT = 6,
    BOTTOM_LEFT = 7,
    BOTTOM_RIGHT = 8,
};

constexpr const char* toString(Anchor e) noexcept {
    constexpr const char* names[] = {
        "CENTER",
        "LEFT",
        "RIGHT",
        "TOP",
        "BOTTOM",
        "TOP_LEFT",
        "TOP_RIGHT",
        "BOTTOM_LEFT",
        "BOTTOM_RIGHT",
    };
    return names[static_cast<int>(e)];
}

namespace std {

template <>
struct hash<::Anchor> {
    size_t operator()(::Anchor type) const {
        return std::hash<int>()(static_cast<int>(type));
    }
};

} // namespace std

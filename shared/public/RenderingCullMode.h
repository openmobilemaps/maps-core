// AUTOGENERATED FILE - DO NOT MODIFY!
// This file was generated by Djinni from core.djinni

#pragma once

#include <functional>

enum class RenderingCullMode : int {
    FRONT = 0,
    BACK = 1,
    NONE = 2,
};

constexpr const char* toString(RenderingCullMode e) noexcept {
    constexpr const char* names[] = {
        "front",
        "back",
        "none",
    };
    return names[static_cast<int>(e)];
}

namespace std {

template <>
struct hash<::RenderingCullMode> {
    size_t operator()(::RenderingCullMode type) const {
        return std::hash<int>()(static_cast<int>(type));
    }
};

} // namespace std

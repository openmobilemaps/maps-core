// AUTOGENERATED FILE - DO NOT MODIFY!
// This file was generated by Djinni from core.djinni

#pragma once

#include <functional>

enum class CameraMode3d : int {
    GLOBE = 0,
    TILTED_ORBITAL = 1,
};

constexpr const char* toString(CameraMode3d e) noexcept {
    constexpr const char* names[] = {
        "globe",
        "tilted_orbital",
    };
    return names[static_cast<int>(e)];
}

namespace std {

template <>
struct hash<::CameraMode3d> {
    size_t operator()(::CameraMode3d type) const {
        return std::hash<int>()(static_cast<int>(type));
    }
};

} // namespace std
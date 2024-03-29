// AUTOGENERATED FILE - DO NOT MODIFY!
// This file was generated by Djinni from core.djinni

#pragma once

#include <functional>

enum class LayerReadyState : int {
    READY = 0,
    NOT_READY = 1,
    ERROR = 2,
    TIMEOUT_ERROR = 3,
};

constexpr const char* toString(LayerReadyState e) noexcept {
    constexpr const char* names[] = {
        "ready",
        "not_ready",
        "error",
        "timeout_error",
    };
    return names[static_cast<int>(e)];
}

namespace std {

template <>
struct hash<::LayerReadyState> {
    size_t operator()(::LayerReadyState type) const {
        return std::hash<int>()(static_cast<int>(type));
    }
};

} // namespace std

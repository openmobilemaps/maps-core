// AUTOGENERATED FILE - DO NOT MODIFY!
// This file was generated by Djinni from task_scheduler.djinni

#pragma once

#include <functional>

enum class TaskPriority : int {
    HIGH = 0,
    NORMAL = 1,
    LOW = 2,
};

constexpr const char* toString(TaskPriority e) noexcept {
    constexpr const char* names[] = {
        "high",
        "normal",
        "low",
    };
    return names[static_cast<int>(e)];
}

namespace std {

template <>
struct hash<::TaskPriority> {
    size_t operator()(::TaskPriority type) const {
        return std::hash<int>()(static_cast<int>(type));
    }
};

} // namespace std

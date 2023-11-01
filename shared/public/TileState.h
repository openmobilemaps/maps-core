#pragma once

enum class TileState : int {
    IN_SETUP = 0, // The tile is in the setup phase, it is not rendered.
    VISIBLE = 1, // The tile is currently visible.
    CACHED = 2, // The tile is cached or stored for later use.
    OUTDATED_VISIBLE = 3 // Tile is visible, but will be replaced once another version of the same tile is ready
};

constexpr const char* toString(TileState e) noexcept {
    constexpr const char* names[] = {
        "IN_SETUP",
        "VISIBLE",
        "CACHED",
        "OUTDATED_VISIBLE"
    };
    return names[static_cast<int>(e)];
}

namespace std {
    template <>
    struct hash<::TileState> {
        size_t operator()(::TileState type) const {
            return std::hash<int>()(static_cast<int>(type));
        }
    };
} // namespace std


#pragma once

enum class TileState : int {
    IN_SETUP = 0, // The tile is in the setup phase, it is not rendered.
    VISIBLE = 1, // The tile is currently visible.
    CACHED = 2 // The tile is cached or stored for later use.
};

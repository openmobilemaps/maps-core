#pragma once

#include <cstdint>

struct ResolvedSpriteIconId {
    uint32_t sheet; // Index of the spritesheet
    uint32_t icon;  // Index of the icon in the sprite description table

    ResolvedSpriteIconId() = default;

    ResolvedSpriteIconId(uint32_t sheet, uint32_t icon)
        : sheet(sheet)
        , icon(icon) {}

    bool operator==(const ResolvedSpriteIconId &o) const { return sheet == o.sheet && icon == o.icon; }
    bool operator!=(const ResolvedSpriteIconId &o) const { return sheet != o.sheet || icon != o.icon; }
};

#pragma once

#include "HashedTuple.h"
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

// TODO do we need hash?
namespace std {
template <> struct hash<ResolvedSpriteIconId> {
    size_t operator()(const ResolvedSpriteIconId &id) const {
        size_t hash = 0;
        hash_combine(hash, std::hash<uint32_t>()(id.sheet));
        hash_combine(hash, std::hash<uint32_t>()(id.icon));
        return hash;
    }
};
} // namespace std

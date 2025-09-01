#pragma once

#include "HashedTuple.h"
#include <string>
#include <ostream>

struct SpriteIconId {
    std::string sheet; // Identifier of the spritesheet
    std::string icon;  // Name of the icon in the spritesheet

    SpriteIconId() = default;

    SpriteIconId(std::string sheet, std::string icon)
        : sheet(std::move(sheet))
        , icon(std::move(icon)) {}

    // Make SpriteIconId by splitting an "icon-image" property at ':' separator
    SpriteIconId(const std::string &iconImage) {
        auto separatorPos = iconImage.find(':');
        if(separatorPos != std::string::npos) {
            sheet = iconImage.substr(0, separatorPos);
            icon = iconImage.substr(separatorPos + 1, iconImage.size() - separatorPos - 1);
        } else {
            // sheet = "";
            icon = iconImage;
        }
    }

    bool empty() const { return sheet.empty() && icon.empty(); }

    bool operator==(const SpriteIconId &o) const { return sheet == o.sheet && icon == o.icon; }
    bool operator!=(const SpriteIconId &o) const { return sheet != o.sheet || icon != o.icon; }
};

namespace std {
template<> struct hash<SpriteIconId> {
    size_t operator()(const SpriteIconId &id) const {
        size_t hash = 0;
        hash_combine(hash, std::hash<std::string>()(id.sheet)); 
        hash_combine(hash, std::hash<std::string>()(id.icon)); 
        return hash;
    }
};
}

inline std::ostream &operator<<(std::ostream &os, const SpriteIconId &id) { return os << id.sheet << ':' << id.icon; }

/*
 * Copyright (c) 2021 Ubique Innovation AG <https://www.ubique.ch>
 *
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this
 *  file, You can obtain one at https://mozilla.org/MPL/2.0/.
 *
 *  SPDX-License-Identifier: MPL-2.0
 */

#pragma once

#include <unordered_map>
#include <string>
#include "json.h"

struct SpriteDesc {
    int x;
    int y;
    int width;
    int height;
    float pixelRatio;

    std::vector<float> content;
    std::vector<std::pair<float,float>> stretchX;
    std::vector<std::pair<float, float>> stretchY;
};

inline void to_json(nlohmann::json& j, const SpriteDesc& spriteDesc) {}

inline void from_json(const nlohmann::json& j, SpriteDesc& spriteDesc) {
    j.at("x").get_to(spriteDesc.x);
    j.at("y").get_to(spriteDesc.y);
    j.at("width").get_to(spriteDesc.width);
    j.at("height").get_to(spriteDesc.height);
    j.at("pixelRatio").get_to(spriteDesc.pixelRatio);

    if (j.contains("content")) {
        j.at("content").get_to(spriteDesc.content);
    }

    if (j.contains("stretchX")) {
        j.at("stretchX").get_to(spriteDesc.stretchX);
    }

    if (j.contains("stretchY")) {
        j.at("stretchY").get_to(spriteDesc.stretchY);
    }
}

struct SpriteData {
    std::string identifier;
    std::unordered_map<std::string, SpriteDesc> sprites;

    SpriteData(std::string identifier, std::unordered_map<std::string, SpriteDesc> sprites)
        : identifier(identifier)
        , sprites(sprites) {}
};

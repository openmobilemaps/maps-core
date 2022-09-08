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
    int pixelRatio;

    NLOHMANN_DEFINE_TYPE_INTRUSIVE(SpriteDesc, x, y, width, height, pixelRatio)
};

struct SpriteData {
    std::unordered_map<std::string, SpriteDesc> sprites;

    SpriteData(std::unordered_map<std::string, SpriteDesc> sprites): sprites(sprites) {}
};

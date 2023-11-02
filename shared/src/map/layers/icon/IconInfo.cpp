/*
 * Copyright (c) 2021 Ubique Innovation AG <https://www.ubique.ch>
 *
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this
 *  file, You can obtain one at https://mozilla.org/MPL/2.0/.
 *
 *  SPDX-License-Identifier: MPL-2.0
 */

#include "IconInfo.h"

IconInfo::IconInfo(const std::string &identifier, const Coord &coordinate, const std::shared_ptr<::TextureHolderInterface> &texture,
                   const Vec2F &iconSize, IconType type, const ::Vec2F &anchor, BlendMode blendMode)
        : identifier(identifier), coordinate(coordinate), texture(texture), iconSize(iconSize), type(type), anchor(anchor),
          blendMode(blendMode) {}

std::string IconInfo::getIdentifier() {
    // Immutable
    return identifier;
}

std::shared_ptr<::TextureHolderInterface> IconInfo::getTexture() {
    // Immutable
    return texture;
}

void IconInfo::setCoordinate(const Coord &coord) {
    std::lock_guard<std::mutex> dataLock(dataMutex);
    this->coordinate = coord;
}

::Coord IconInfo::getCoordinate() {
    std::lock_guard<std::mutex> dataLock(dataMutex);
    return coordinate;
}

void IconInfo::setIconSize(const Vec2F &size) {
    std::lock_guard<std::mutex> dataLock(dataMutex);
    this->iconSize = size;
}

::Vec2F IconInfo::getIconSize() {
    std::lock_guard<std::mutex> dataLock(dataMutex);
    return iconSize;
}

void IconInfo::setType(IconType type) {
    std::lock_guard<std::mutex> dataLock(dataMutex);
    this->type = type;
}

IconType IconInfo::getType() {
    std::lock_guard<std::mutex> dataLock(dataMutex);
    return type;
}

::Vec2F IconInfo::getIconAnchor() {
    // Immutable
    return anchor;
}

BlendMode IconInfo::getBlendMode() {
    // Immutable
    return blendMode;
}

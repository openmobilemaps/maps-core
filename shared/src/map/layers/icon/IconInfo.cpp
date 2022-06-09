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
                   const Vec2F &iconSize, IconType type, const ::Vec2F &anchor)
    : identifier(identifier)
    , coordinate(coordinate)
    , texture(texture)
    , iconSize(iconSize)
    , type(type)
    , anchor(anchor) {}

std::string IconInfo::getIdentifier() { return identifier; }

std::shared_ptr<::TextureHolderInterface> IconInfo::getTexture() { return texture; }

void IconInfo::setCoordinate(const Coord &coord) { this->coordinate = coord; }

::Coord IconInfo::getCoordinate() { return coordinate; }

void IconInfo::setIconSize(const Vec2F &size) { this->iconSize = size; }

::Vec2F IconInfo::getIconSize() { return iconSize; }

void IconInfo::setType(IconType type) { this->type = type; }

IconType IconInfo::getType() { return type; }

::Vec2F IconInfo::getIconAnchor() { return anchor; }

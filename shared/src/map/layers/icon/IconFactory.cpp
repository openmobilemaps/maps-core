/*
 * Copyright (c) 2021 Ubique Innovation AG <https://www.ubique.ch>
 *
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this
 *  file, You can obtain one at https://mozilla.org/MPL/2.0/.
 *
 *  SPDX-License-Identifier: MPL-2.0
 */

#include "IconFactory.h"
#include "IconInfo.h"

std::shared_ptr<IconInfoInterface> IconFactory::createIcon(const std::string &identifier, const ::Coord &coordinate,
                                                             const std::shared_ptr<::TextureHolderInterface> &texture,
                                                             const ::Vec2F &iconSize, IconType type) {
    return std::make_shared<IconInfo>(identifier, coordinate, texture, iconSize, type);
}
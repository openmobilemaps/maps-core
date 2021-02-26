/*
 * Copyright (c) 2021 Ubique Innovation AG <https://www.ubique.ch>
 *
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this
 *  file, You can obtain one at https://mozilla.org/MPL/2.0/.
 *
 *  SPDX-License-Identifier: MPL-2.0
 */

#include "IconInfoInterface.h"
#include "IconInfo.h"

std::shared_ptr<IconInfoInterface> IconInfoInterface::create(const std::string &identifier, const ::Coord &coordinate,
                                                             const std::shared_ptr<::TextureHolderInterface> &texture,
                                                             const ::Vec2F &iconSize, IconScaleType scaleType) {
    return std::make_shared<IconInfo>(identifier, coordinate, texture, iconSize, scaleType);
}
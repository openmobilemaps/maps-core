/*
 * Copyright (c) 2021 Ubique Innovation AG <https://www.ubique.ch>
 *
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this
 *  file, You can obtain one at https://mozilla.org/MPL/2.0/.
 *
 *  SPDX-License-Identifier: MPL-2.0
 */

#include "TextFactory.h"

#include "TextInfo.h"

std::shared_ptr<TextInfoInterface> TextFactory::createText(const std::vector<FormattedStringEntry> &text, const ::Coord &coordinate, const ::Font &font, Anchor textAnchor, TextJustify textJustify) {
    return std::make_shared<TextInfo>(text, coordinate, font, textAnchor, textJustify);
}

/*
 * Copyright (c) 2021 Ubique Innovation AG <https://www.ubique.ch>
 *
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this
 *  file, You can obtain one at https://mozilla.org/MPL/2.0/.
 *
 *  SPDX-License-Identifier: MPL-2.0
 */

#include "LineFactory.h"
#include "LineInfo.h"

std::shared_ptr<LineInfoInterface> LineFactory::createLine(const std::string & identifier,
                                                           const std::vector<::Coord> & coordinates,
                                                           const LineStyle & style) {
    return std::make_shared<LineInfo>(identifier, coordinates, style);
}

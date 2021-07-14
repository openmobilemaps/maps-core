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

#include "LineInfoInterface.h"
#include "LineStyle.h"

class LineInfo : public LineInfoInterface {
public:
    LineInfo(const std::string & identifier, const std::vector<::Coord> & coordinates, const LineStyle style) :
        identifier(identifier),
        coordinates(coordinates),
        style(style) {};

    virtual ~LineInfo() {}

    virtual std::string getIdentifier() override { return identifier; };

    virtual std::vector<::Coord> getCoordinates() override { return coordinates; };

    virtual LineStyle getStyle() override { return style; };

private:
    std::string identifier;
    std::vector<::Coord> coordinates;
    LineStyle style;
};

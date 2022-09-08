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

#include "GraphicsObjectInterface.h"
#include "TextInfoInterface.h"
#include "Coord.h"
#include "FontLoaderInterface.h"
#include "Anchor.h"
#include "Vec2F.h"
#include "Value.h"

class SymbolInfo : public TextInfoInterface {
public:
    SymbolInfo(const std::vector<FormattedStringEntry> &text,
               const ::Coord &coordinate,
               const ::Font &font,
               Anchor textAnchor,
               std::optional<double> angle)
            : text(text), coordinate(coordinate), font(font), textAnchor(textAnchor), angle(angle) {};

    // Text Interface
    virtual ~SymbolInfo() {};

    virtual std::vector<FormattedStringEntry> getText() { return text; };

    virtual ::Coord getCoordinate() { return coordinate; };

    virtual ::Font getFont() { return font; };

    virtual Anchor getTextAnchor() { return textAnchor; };

    std::optional<double> angle;

private:
    std::vector<FormattedStringEntry> text;
    ::Font font;
    ::Coord coordinate;
    ::Anchor textAnchor;
};

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
#include "TextJustify.h"
#include "Vec2F.h"
#include "Value.h"

class SymbolInfo : public TextInfoInterface {
public:
    SymbolInfo(const std::vector<FormattedStringEntry> &text,
               const ::Coord &coordinate,
               const std::optional<std::vector<Coord>> &lineCoordinates,
               const ::Font &font,
               Anchor textAnchor,
               std::optional<double> angle,
               TextJustify textJustify,
               TextSymbolPlacement textSymbolPlacement)
            : text(text), coordinate(coordinate), font(font), textAnchor(textAnchor), angle(angle), textJustify(textJustify), lineCoordinates(lineCoordinates), textSymbolPlacement(textSymbolPlacement) {};

    // Text Interface
    virtual ~SymbolInfo() {};

    virtual std::vector<FormattedStringEntry> getText() { return text; };

    virtual ::Coord getCoordinate() { return coordinate; };

    virtual ::Font getFont() { return font; };

    virtual Anchor getTextAnchor() { return textAnchor; };

    virtual TextJustify getTextJustify() { return textJustify; };

    virtual TextSymbolPlacement getSymbolPlacement() { return textSymbolPlacement; }

    virtual std::optional<std::vector<::Coord>> getLineCoordinates() { return lineCoordinates; }

    std::optional<double> angle;

private:
    std::vector<FormattedStringEntry> text;
    ::Font font;
    ::Coord coordinate;
    ::Anchor textAnchor;
    ::TextJustify textJustify;
    ::TextSymbolPlacement textSymbolPlacement;
    std::optional<std::vector<Coord>> lineCoordinates;
};

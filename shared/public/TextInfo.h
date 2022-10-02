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
#include "FormattedStringEntry.h"

class TextInfo : public TextInfoInterface {
  public:
    TextInfo(const std::vector<FormattedStringEntry> &text, const ::Coord &coordinate, const ::Font &font, Anchor textAnchor, TextJustify textJustify)
        : text(text)
        , coordinate(coordinate)
        , font(font)
        , textAnchor(textAnchor)
        , textJustify(textJustify) {};

    // Text Interface
    virtual ~TextInfo(){};

    virtual std::vector<FormattedStringEntry> getText() { return text; };

    virtual ::Coord getCoordinate() { return coordinate; };

    virtual ::Font getFont() { return font; };

    virtual Anchor getTextAnchor() { return textAnchor; };

    virtual TextJustify getTextJustify() { return textJustify; };

  private:
    std::vector<FormattedStringEntry> text;
    ::Font font;
    ::Coord coordinate;
    ::Anchor textAnchor;
    ::TextJustify textJustify;
};

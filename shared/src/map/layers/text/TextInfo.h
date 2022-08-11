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

class TextInfo : public TextInfoInterface {
  public:
    TextInfo(const std::string &text, const ::Coord &coordinate, const ::Font &font)
        : text(text)
        , coordinate(coordinate)
        , font(font){};

    // Text Interface
    virtual ~TextInfo(){};

    virtual std::string getText() { return text; };

    virtual ::Coord getCoordinate() { return coordinate; };

    virtual ::Font getFont() { return font; };

  private:
    std::string text;
    ::Font font;
    ::Coord coordinate;
};

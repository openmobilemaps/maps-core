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

#include "VectorLayerDescription.h"
#include "Color.h"
#include "Font.h"
#include "Vec2F.h"

enum TextTransform {
    NONE,
    UPPERCASE
};

class SymbolVectorStyle {
public:
    std::shared_ptr<Value<float>> size;
    Font font;
    std::string textField;
    TextTransform transform;
    Vec2F textOffset;
    Color color;

    SymbolVectorStyle(std::shared_ptr<Value<float>> size, Color color, const std::string &fontName, const std::string &textField, TextTransform transform, Vec2F textOffset):
    size(size),
    font(fontName),
    textField(textField),
    transform(transform),
    textOffset(textOffset),
    color(color) {}
};

class SymbolVectorLayerDescription: public VectorLayerDescription {
public:
    VectorLayerType getType() override { return VectorLayerType::symbol; };
    SymbolVectorStyle style;

    SymbolVectorLayerDescription(std::string identifier,
                               std::string sourceId,
                               int minZoom,
                               int maxZoom,
                               std::shared_ptr<Value<bool>> filter,
                               SymbolVectorStyle style):
    VectorLayerDescription(identifier, sourceId, minZoom, maxZoom, filter),
    style(style) {};
};

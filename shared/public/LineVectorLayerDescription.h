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
#include "LineCapType.h"

class LineVectorStyle {
public:
    std::shared_ptr<Value<Color>> color;
    std::shared_ptr<Value<float>> opacity;
    std::shared_ptr<Value<float>> width;
    std::shared_ptr<Value<std::vector<float>>> dashArray;
    LineCapType capType;
    LineVectorStyle(std::shared_ptr<Value<Color>> color = nullptr,
                    std::shared_ptr<Value<float>> opacity = nullptr,
                    std::shared_ptr<Value<float>> width = nullptr,
                    std::shared_ptr<Value<std::vector<float>>> dashArray = nullptr,
                    LineCapType capType = LineCapType::ROUND):
    color(color),
    opacity(opacity),
    width(width),
    dashArray(dashArray),
    capType(capType) {}
};

class LineVectorLayerDescription: public VectorLayerDescription {
public:
    VectorLayerType getType() override { return VectorLayerType::line; };
    LineVectorStyle style;

    LineVectorLayerDescription(std::string identifier,
                               std::string sourceId,
                               int minZoom,
                               int maxZoom,
                               std::shared_ptr<Value<bool>> filter,
                               LineVectorStyle style):
    VectorLayerDescription(identifier, sourceId, minZoom, maxZoom, filter),
    style(style) {};

};

